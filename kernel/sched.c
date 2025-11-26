#include "uros.h"

extern pcb_t *task_get_table(void);
extern int task_get_max_tasks(void);

static sched_mode_t current_mode = SCHED_RR;
static pcb_t *current_task = (pcb_t *)0;
static int quantum_left = RR_QUANTUM;
static int preempt_enabled = CONFIG_PREEMPT_DEFAULT;

// Simple ready queue (array of task pointers)
static pcb_t *ready_queue[MAX_TASKS];
static int ready_head = 0;
static int ready_tail = 0;
static int ready_count = 0;

void sched_init(void) {
  current_mode = SCHED_RR;
  current_task = (pcb_t *)0;
  quantum_left = RR_QUANTUM;
  ready_head = 0;
  ready_tail = 0;
  ready_count = 0;
}

void sched_add_ready(pcb_t *task) {
  if (!task || ready_count >= MAX_TASKS) {
    return;
  }

  u64 sstatus;
  __asm__ volatile("csrr %0, sstatus" : "=r"(sstatus));
  int interrupts_were_enabled = (sstatus & (1UL << 1)) != 0;
  if (interrupts_were_enabled) {
    disable_irq();
  }

  ready_queue[ready_tail] = task;
  ready_tail = (ready_tail + 1) % MAX_TASKS;
  ready_count++;
  task->state = TASK_READY;
  need_resched = 1;

  if (interrupts_were_enabled) {
    enable_irq();
  }
}

static pcb_t *sched_pick_next_rr(void) {
  if (ready_count == 0) {
    return (pcb_t *)0;
  }

  pcb_t *task = ready_queue[ready_head];
  ready_head = (ready_head + 1) % MAX_TASKS;
  ready_count--;

  quantum_left = RR_QUANTUM;
  return task;
}

static pcb_t *sched_pick_next_sjf(void) {
  if (ready_count == 0) {
    return (pcb_t *)0;
  }

  // Find task with minimum burst estimate
  int best_idx = -1;
  u64 min_burst = ~0UL;

  for (int i = 0; i < ready_count; i++) {
    int idx = (ready_head + i) % MAX_TASKS;
    pcb_t *task = ready_queue[idx];

    if (task->burst_estimate < min_burst ||
        (task->burst_estimate == min_burst &&
         (best_idx == -1 ||
          task->arrival_time < ready_queue[best_idx]->arrival_time))) {
      min_burst = task->burst_estimate;
      best_idx = idx;
    }
  }

  if (best_idx == -1) {
    return (pcb_t *)0;
  }

  // Remove from queue
  pcb_t *task = ready_queue[best_idx];

  // Shift queue to remove the selected task
  for (int i = 0; i < ready_count - 1; i++) {
    int src_idx = (best_idx + 1 + i) % MAX_TASKS;
    int dst_idx = (best_idx + i) % MAX_TASKS;
    ready_queue[dst_idx] = ready_queue[src_idx];
  }

  ready_count--;
  ready_tail = (ready_tail - 1 + MAX_TASKS) % MAX_TASKS;

  return task;
}

void sched_yield(void) {
  disable_irq();

  pcb_t *prev = current_task;
  pcb_t *next;

  // Pick next task based on mode
  if (current_mode == SCHED_RR) {
    next = sched_pick_next_rr();
  } else {
    next = sched_pick_next_sjf();
  }

  // If no task available, stay idle or re-run current
  if (!next) {
    enable_irq();
    return;
  }

  // Save current task back to ready queue if still runnable
  if (prev && prev->state == TASK_RUNNING) {
    prev->state = TASK_READY;
    sched_add_ready(prev);
  }

  // Update task metrics
  if (next->start_time == 0) {
    next->start_time = g_ticks;
    next->wait_time = g_ticks - next->arrival_time;
  }

  next->state = TASK_RUNNING;
  current_task = next;

  // Clear resched flag
  need_resched = 0;

  enable_irq();

  // Context switch
  if (prev && prev != next) {
    ctx_switch(&prev->context, &next->context);
  } else if (!prev && next) {
    // First task - no previous context to save
    ctx_switch((context_t *)0, &next->context);
  }
}

void sched_on_tick(void) {
  if (!current_task) {
    if (ready_count > 0) {
      need_resched = 1;
    }
    return;
  }

  if (current_task->pid != 0) {
    current_task->ticks_used++;
  }

  if (current_mode == SCHED_RR) {
    if (preempt_enabled) {
      if (--quantum_left <= 0) {
        need_resched = 1;
        quantum_left = RR_QUANTUM;
      }
    }
  } else {
    // SJF is non-preemptive; if idle is running but work exists, request
    // resched
    if (current_task->pid == 0 && ready_count > 0) {
      need_resched = 1;
    }
  }
}

void sched_set_mode(sched_mode_t mode) {
  disable_irq();
  current_mode = mode;
  if (mode == SCHED_RR) {
    quantum_left = RR_QUANTUM;
  }
  enable_irq();
}

sched_mode_t sched_get_mode(void) { return current_mode; }

pcb_t *sched_current(void) { return current_task; }

void sched_update_burst_estimate(pcb_t *task) {
  if (!task) {
    return;
  }

  // Exponential averaging: τ_new = α * real_burst + (1-α) * τ_old
  // α = 0.5
  u64 real_burst = task->ticks_used;
  task->burst_estimate = (real_burst + task->burst_estimate) / 2;
}

// Cooperative scheduling - call this periodically from user code
void sched_maybe_yield_safe(void) {
  if (!need_resched) {
    return;
  }

  if (current_task) {
    need_resched = 0;
    sched_yield();
  } else {
    need_resched = 0;
  }
}

// Set preemption mode
void sched_set_preempt(int on) {
  disable_irq();
  preempt_enabled = on ? 1 : 0;
  if (preempt_enabled && current_mode == SCHED_RR) {
    need_resched = 1;
    quantum_left = RR_QUANTUM;
  }
  enable_irq();
}

// Get preemption mode
int sched_get_preempt(void) { return preempt_enabled; }
