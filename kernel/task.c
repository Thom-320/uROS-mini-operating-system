#include "uros.h"

// Task table
static pcb_t tasks[MAX_TASKS];
static int next_pid = 0;

void task_init(void) {
  // Initialize all tasks to ZOMBIE (free)
  for (int i = 0; i < MAX_TASKS; i++) {
    tasks[i].state = TASK_ZOMBIE;
    tasks[i].pid = -1;
  }
  next_pid = 0;
}

// Task entry wrapper
static void task_entry_wrapper(void) {
  pcb_t *current = sched_current();

  if (current && current->entry) {
    // Call the actual task function
    current->entry(current->arg);
  }

  // Task finished - exit
  task_exit();
}

int task_create(void (*entry)(void *), void *arg, int burst_hint) {
  disable_irq();

  // Find free PID
  int pid = -1;
  for (int i = 0; i < MAX_TASKS; i++) {
    if (tasks[i].state == TASK_ZOMBIE) {
      pid = i;
      break;
    }
  }

  if (pid == -1) {
    enable_irq();
    return -1; // No free slots
  }

  // Allocate stack (aligned to 16 bytes)
  void *stack = kmalloc(STACK_SIZE);
  if (!stack) {
    enable_irq();
    return -1;
  }

  // Initialize PCB
  pcb_t *task = &tasks[pid];
  memset(task, 0, sizeof(pcb_t));

  task->pid = pid;
  task->state = TASK_NEW;
  task->stack_base = stack;
  task->entry = entry;
  task->arg = arg;
  task->burst_hint = burst_hint;
  task->burst_estimate = burst_hint;
  task->arrival_time = g_ticks;
  task->ticks_used = 0;
  task->start_time = 0;
  task->finish_time = 0;
  task->wait_time = 0;

  // Initialize context
  memset(&task->context, 0, sizeof(context_t));

  // Set up initial context
  // Stack grows down, align to 16 bytes
  u64 stack_top = (u64)stack + STACK_SIZE;
  stack_top &= ~15UL; // Align to 16 bytes

  task->context.x1 = (u64)task_entry_wrapper;   // ra used by ctx_switch->ret
  task->context.x2 = stack_top;                 // sp
  task->context.sepc = (u64)task_entry_wrapper; // pc (for reference)
  task->context.x10 = (u64)arg;                 // a0 (first argument)

  // Set sstatus: enable interrupts when task runs
  // SPP=1 (S-mode return), SPIE=1 (Previous IE), SIE=1 (Enable interrupts
  // immediately) We set SIE=1 because ctx_switch restores sstatus directly via
  // csrw, not sret.
  task->context.sstatus = 0x00000122;

  // Add to scheduler
  task->state = TASK_READY;
  sched_add_ready(task);

  enable_irq();

  return pid;
}

void task_exit(void) {
  disable_irq();

  pcb_t *current = sched_current();
  if (current) {
    current->state = TASK_ZOMBIE;
    current->finish_time = g_ticks;
  }

  enable_irq();

  // Yield to next task
  sched_yield();

  // Should never reach here
  while (1)
    ;
}

void task_yield(void) { sched_yield(); }

pcb_t *task_get_by_pid(int pid) {
  if (pid < 0 || pid >= MAX_TASKS) {
    return (pcb_t *)0;
  }

  if (tasks[pid].state == TASK_ZOMBIE && tasks[pid].pid == -1) {
    return (pcb_t *)0;
  }

  return &tasks[pid];
}

void task_reap(int pid) {
  if (pid < 0 || pid >= MAX_TASKS) {
    return;
  }

  // Don't kill idle task (PID 0)
  if (pid == 0) {
    return;
  }

  pcb_t *task = &tasks[pid];
  if (task->state == TASK_ZOMBIE && task->stack_base) {
    // Free the stack memory
    kfree(task->stack_base);
    task->stack_base = (void *)0;

    // Mark as completely free
    task->pid = -1;
    task->state = TASK_ZOMBIE;
  }
}

// Get task table for ps command
pcb_t *task_get_table(void) { return tasks; }

int task_get_max_tasks(void) { return MAX_TASKS; }

// Idle task - runs when no other tasks are ready
void idle_task(void *arg) {
  (void)arg; // Unused parameter

  while (1) {
    task_yield();
    __asm__ volatile("wfi");
  }
}
