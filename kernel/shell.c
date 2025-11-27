#include "uros.h"

extern pcb_t *task_get_table(void);
extern int task_get_max_tasks(void);
extern void sched_update_burst_estimate(pcb_t *task);

static inline u64 csr_read_sstatus(void) {
  u64 value;
  __asm__ volatile("csrr %0, sstatus" : "=r"(value));
  return value;
}

static inline u64 csr_read_sie(void) {
  u64 value;
  __asm__ volatile("csrr %0, sie" : "=r"(value));
  return value;
}

static inline u64 csr_read_sip(void) {
  u64 value;
  __asm__ volatile("csrr %0, sip" : "=r"(value));
  return value;
}

// Producer-Consumer shared data
#define BUFFER_SIZE 16
static int pc_buffer[BUFFER_SIZE];
static int pc_in = 0;
static int pc_out = 0;
static sem_t pc_empty;
static sem_t pc_full;
static mutex_t pc_mutex;

// Task functions for run commands
static void cpu_task(void *arg) {
  int iterations = (int)(u64)arg;

  for (int i = 0; i < iterations; i++) {
    // Burn CPU for a bit
    volatile int sum = 0;
    for (int j = 0; j < 10000; j++) {
      sum += j;
    }

    // Cooperative scheduling check
    sched_maybe_yield_safe();

    // Yield occasionally
    if (i % 5 == 0) {
      task_yield();
    }
  }
}

static void io_task(void *arg) {
  int id = (int)(u64)arg;

  for (int i = 0; i < 5; i++) {
    kprintf("IO task %d: iteration %d\n", id, i);

    // Simulate I/O wait by yielding and letting other tasks run
    for (int j = 0; j < 5; j++) {
      sched_maybe_yield_safe();
      task_yield();
    }
  }
}

// Benchmark task
static void bench_task(void *arg) {
  int burst_ticks = (int)(u64)arg;
  u64 start = g_ticks;

  // Busy wait for the specified number of ticks
  while ((g_ticks - start) < (u64)burst_ticks) {
    // Burn CPU
    volatile int sum = 0;
    for (int j = 0; j < 1000; j++) {
      sum += j;
    }
    // Optional: Yield to allow other tasks to run if cooperative
    // task_yield();
  }

  // Task completes
}

// Producer task for producer-consumer demo
static void producer_task(void *arg) {
  int n_items = (int)(u64)arg;

  for (int i = 0; i < n_items; i++) {
    // Wait for empty slot
    sem_wait(&pc_empty);

    // Enter critical section
    mutex_lock(&pc_mutex);

    // Produce item
    int item = i + 1;
    pc_buffer[pc_in] = item;
    kprintf("Producer: produced item %d at index %d\n", item, pc_in);
    pc_in = (pc_in + 1) % BUFFER_SIZE;

    // Exit critical section
    mutex_unlock(&pc_mutex);

    // Signal item available
    sem_post(&pc_full);

    // Sleep to make demo visible
    u64 target = g_ticks + 5;
    while (g_ticks < target) {
      sched_maybe_yield_safe();
      task_yield();
    }
  }

  kprintf("Producer: finished producing %d items\n", n_items);
}

// Consumer task for producer-consumer demo
static void consumer_task(void *arg) {
  int n_items = (int)(u64)arg;

  for (int i = 0; i < n_items; i++) {
    // Wait for item available
    sem_wait(&pc_full);

    // Enter critical section
    mutex_lock(&pc_mutex);

    // Consume item
    int item = pc_buffer[pc_out];
    kprintf("Consumer: consumed item %d from index %d\n", item, pc_out);
    pc_out = (pc_out + 1) % BUFFER_SIZE;

    // Exit critical section
    mutex_unlock(&pc_mutex);

    // Signal empty slot
    sem_post(&pc_empty);

    // Sleep to make demo visible
    u64 target = g_ticks + 5;
    while (g_ticks < target) {
      sched_maybe_yield_safe();
      task_yield();
    }
  }

  kprintf("Consumer: finished consuming %d items\n", n_items);
}

static void cmd_help(void) {
  kprintf("Available commands:\n");
  kprintf("  help            - Show this help\n");
  kprintf("  ps              - List tasks\n");
  kprintf("  run cpu         - Create CPU-bound task\n");
  kprintf("  run io          - Create I/O-bound task\n");
  kprintf("  kill <pid>      - Kill a task\n");
  kprintf("  sched rr        - Switch to Round-Robin\n");
  kprintf("  sched sjf       - Switch to SJF\n");
  kprintf("  sched preempt on|off - Enable/disable preemption\n");
  kprintf("  sleep <ticks>   - Sleep for N ticks\n");
  kprintf("  pcdemo          - Producer-Consumer demo\n");
  kprintf("  bench           - Run scheduler benchmark\n");
  kprintf("  uptime          - Show system uptime\n");
  kprintf("  meminfo         - Show memory usage\n");
  kprintf("  intstats        - Show interrupt/timer status\n");
}

static void cmd_ps(void) {
  pcb_t *tasks = task_get_table();
  int max_tasks = task_get_max_tasks();

  kprintf("PID  STATE     TICKS  BURST_EST  ARRIVAL\n");

  for (int i = 0; i < max_tasks; i++) {
    if (tasks[i].pid >= 0 && tasks[i].state != TASK_ZOMBIE) {
      const char *state_str;
      switch (tasks[i].state) {
      case TASK_NEW:
        state_str = "NEW    ";
        break;
      case TASK_READY:
        state_str = "READY  ";
        break;
      case TASK_RUNNING:
        state_str = "RUNNING";
        break;
      case TASK_SLEEPING:
        state_str = "SLEEP  ";
        break;
      default:
        state_str = "ZOMBIE ";
        break;
      }

      kprintf("%d    %s  %u      %u          %u\n", tasks[i].pid, state_str,
              (u32)tasks[i].ticks_used, (u32)tasks[i].burst_estimate,
              (u32)tasks[i].arrival_time);
    }
  }
}

static void cmd_run_cpu(void) {
  int pid = task_create(cpu_task, (void *)50, 20);
  if (pid >= 0) {
    kprintf("Created CPU task with PID %d\n", pid);
  } else {
    kprintf("Failed to create task\n");
  }
}

static void cmd_run_io(void) {
  static int io_counter = 0;
  int pid = task_create(io_task, (void *)(u64)io_counter++, 15);
  if (pid >= 0) {
    kprintf("Created I/O task with PID %d\n", pid);
  } else {
    kprintf("Failed to create task\n");
  }
}

static void cmd_kill(const char *arg) {
  int pid = atoi(arg);
  pcb_t *task = task_get_by_pid(pid);

  if (task) {
    task->state = TASK_ZOMBIE;
    task->finish_time = g_ticks;
    task_reap(pid);
    kprintf("Killed task %d\n", pid);
  } else {
    kprintf("Task %d not found\n", pid);
  }
}

static void cmd_sched(const char *arg) {
  if (strcmp(arg, "rr") == 0) {
    sched_set_mode(SCHED_RR);
    kprintf("Scheduler: Round-Robin (quantum=%d ticks)\n", RR_QUANTUM);
  } else if (strcmp(arg, "sjf") == 0) {
    sched_set_mode(SCHED_SJF);
    kprintf("Scheduler: Shortest Job First (non-preemptive)\n");
  } else if (strncmp(arg, "preempt ", 8) == 0) {
    const char *mode = arg + 8;
    if (strcmp(mode, "on") == 0) {
      sched_set_preempt(1);
      kprintf("Preemption: ENABLED (timer-driven context switches)\n");
    } else if (strcmp(mode, "off") == 0) {
      sched_set_preempt(0);
      kprintf("Preemption: DISABLED (cooperative scheduling)\n");
    } else {
      kprintf("Usage: sched preempt on|off\n");
    }
  } else {
    kprintf("Usage: sched rr|sjf|preempt on|off\n");
  }
}

static void cmd_uptime(void) {
  u64 ticks = g_ticks;
  u64 seconds = ticks / TICK_HZ;
  u64 centisecs = (ticks % TICK_HZ);

  kprintf("Uptime: %u.%u seconds (%u ticks)\n", (u32)seconds, (u32)centisecs,
          (u32)ticks);
}

static void cmd_meminfo(void) {
  size_t used = kmalloc_used();
  size_t free = kmalloc_free();
  int free_blocks = kmalloc_free_blocks();
  size_t total = HEAP_SIZE;

  kprintf("=== Memory Usage ===\n");
  kprintf("Heap total:    %u bytes\n", (u32)total);
  kprintf("Heap used:     %u bytes (%u%%)\n", (u32)used,
          (u32)((used * 100) / total));
  kprintf("Heap free:     %u bytes (%u%%)\n", (u32)free,
          (u32)((free * 100) / total));
  kprintf("Free blocks:   %d\n", free_blocks);
  kprintf("Fragmentation: %s\n", free_blocks > 3 ? "moderate" : "low");
}

static void cmd_intstats(void) {
  u64 sstatus = csr_read_sstatus();
  u64 sie = csr_read_sie();
  u64 sip = csr_read_sip();

  kprintf("ticks=%u  sstatus=0x%x  sie=0x%x  sip=0x%x\n", (u32)g_ticks,
          (u64)sstatus, (u64)sie, (u64)sip);
  kprintf("preempt=%s\n", sched_get_preempt() ? "ON" : "OFF");
}

static void cmd_sleep(const char *arg) {
  // Parse number of ticks
  int ticks = 0;
  for (const char *p = arg; *p >= '0' && *p <= '9'; p++) {
    ticks = ticks * 10 + (*p - '0');
  }

  if (ticks <= 0) {
    kprintf("Usage: sleep <ticks>\n");
    return;
  }

  kprintf("Sleeping for %d ticks...\n", ticks);
  u64 target = g_ticks + ticks;

  while (g_ticks < target) {
    if (!sched_get_preempt()) {
      // Cooperative mode: must yield
      sched_maybe_yield_safe();
      task_yield();
    }
    // Preemptive mode: timer will context switch automatically
  }

  kprintf("Done sleeping\n");
}

static void cmd_pcdemo(void) {
  kprintf("=== Producer-Consumer Demo ===\n");
  kprintf("Buffer size: %d items\n", BUFFER_SIZE);

  int n_items = 10; // Number of items to produce/consume

  // Reset buffer indices
  pc_in = 0;
  pc_out = 0;

  // Initialize synchronization primitives
  sem_init(&pc_empty, BUFFER_SIZE); // Initially all slots empty
  sem_init(&pc_full, 0);            // Initially no items
  mutex_init(&pc_mutex);

  kprintf("Creating producer and consumer tasks...\n");

  // Create producer task
  int producer_pid = task_create(producer_task, (void *)(u64)n_items, 20);
  if (producer_pid < 0) {
    kprintf("Error: failed to create producer task\n");
    return;
  }
  kprintf("Producer task created (PID %d)\n", producer_pid);

  // Create consumer task
  int consumer_pid = task_create(consumer_task, (void *)(u64)n_items, 20);
  if (consumer_pid < 0) {
    kprintf("Error: failed to create consumer task\n");
    return;
  }
  kprintf("Consumer task created (PID %d)\n", consumer_pid);

  kprintf("Demo running... (will produce/consume %d items)\n", n_items);
  kprintf("Watch the alternating output!\n");
}

static void cmd_bench(void) {
  kprintf("Running benchmark...\n");

  // Burst times for 6 tasks
  int bursts[] = {10, 20, 15, 30, 25, 12};
  int num_tasks = 6;

  // Metrics
  u64 wait_rr = 0, turnaround_rr = 0, duration_rr = 0;
  u64 wait_sjf = 0, turnaround_sjf = 0, duration_sjf = 0;

  // Round 1: RR
  kprintf("Round 1: Round-Robin...\n");

  disable_irq();
  sched_set_mode(SCHED_RR);
  enable_irq();

  u64 start_rr = g_ticks;
  int pids_rr[6];

  for (int i = 0; i < num_tasks; i++) {
    pids_rr[i] = task_create(bench_task, (void *)(u64)bursts[i], bursts[i]);
  }

  // Wait for all tasks to finish
  int all_done = 0;
  while (!all_done) {
    all_done = 1;
    for (int i = 0; i < num_tasks; i++) {
      pcb_t *task = task_get_by_pid(pids_rr[i]);
      if (task && task->state != TASK_ZOMBIE) {
        all_done = 0;
        // kprintf("Waiting for task %d (State: %d)\n", pids_rr[i],
        // task->state);
        break;
      }
    }
    task_yield();
  }

  duration_rr = g_ticks - start_rr;

  // Collect metrics
  for (int i = 0; i < num_tasks; i++) {
    pcb_t *task = task_get_by_pid(pids_rr[i]);
    if (task) {
      wait_rr += task->wait_time;
      turnaround_rr += (task->finish_time - task->arrival_time);
      sched_update_burst_estimate(task);
      task_reap(pids_rr[i]);
    }
  }

  kprintf("RR done in %u ticks\n", (u32)duration_rr);

  // Small delay
  for (volatile int i = 0; i < 100000; i++)
    ;

  // Round 2: SJF
  kprintf("Round 2: SJF...\n");

  disable_irq();
  sched_set_mode(SCHED_SJF);
  enable_irq();

  u64 start_sjf = g_ticks;
  int pids_sjf[6];

  for (int i = 0; i < num_tasks; i++) {
    pids_sjf[i] = task_create(bench_task, (void *)(u64)bursts[i], bursts[i]);
  }

  // Wait for all tasks to finish
  all_done = 0;
  while (!all_done) {
    all_done = 1;
    for (int i = 0; i < num_tasks; i++) {
      pcb_t *task = task_get_by_pid(pids_sjf[i]);
      if (task && task->state != TASK_ZOMBIE) {
        all_done = 0;
        break;
      }
    }
    task_yield();
  }

  duration_sjf = g_ticks - start_sjf;

  // Collect metrics
  for (int i = 0; i < num_tasks; i++) {
    pcb_t *task = task_get_by_pid(pids_sjf[i]);
    if (task) {
      wait_sjf += task->wait_time;
      turnaround_sjf += (task->finish_time - task->arrival_time);
      sched_update_burst_estimate(task);
      task_reap(pids_sjf[i]);
    }
  }

  kprintf("SJF done in %u ticks\n", (u32)duration_sjf);

  // Print comparison table
  kprintf("\nBenchmark Results (%d tasks):\n", num_tasks);
  kprintf("                  RR        SJF\n");
  kprintf("Wait (avg):       %u        %u ticks\n", (u32)(wait_rr / num_tasks),
          (u32)(wait_sjf / num_tasks));
  kprintf("Turnaround (avg): %u        %u ticks\n",
          (u32)(turnaround_rr / num_tasks), (u32)(turnaround_sjf / num_tasks));

  // Throughput: tasks per 100 ticks (to avoid float)
  u32 throughput_rr = (num_tasks * 100) / (duration_rr ? duration_rr : 1);
  u32 throughput_sjf = (num_tasks * 100) / (duration_sjf ? duration_sjf : 1);
  kprintf("Throughput:       %u.%u     %u.%u tasks/sec\n", throughput_rr / 100,
          throughput_rr % 100, throughput_sjf / 100, throughput_sjf % 100);
}

void shell_run(void) {
  char buf[128];

  while (1) {
    sched_maybe_yield_safe();

    kprintf("HeliOS> ");
    uart_gets(buf, 64);

    // Trim newline
    int len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
      buf[len - 1] = '\0';
    }

    sched_maybe_yield_safe();

    // Skip empty lines
    if (buf[0] == '\0') {
      sched_maybe_yield_safe();
      continue;
    }

    // Parse command
    if (strcmp(buf, "help") == 0) {
      cmd_help();
    } else if (strcmp(buf, "ps") == 0) {
      cmd_ps();
    } else if (strcmp(buf, "run cpu") == 0) {
      cmd_run_cpu();
    } else if (strcmp(buf, "run io") == 0) {
      cmd_run_io();
    } else if (strncmp(buf, "kill ", 5) == 0) {
      cmd_kill(buf + 5);
    } else if (strncmp(buf, "sched ", 6) == 0) {
      cmd_sched(buf + 6);
    } else if (strcmp(buf, "pcdemo") == 0) {
      cmd_pcdemo();
    } else if (strcmp(buf, "bench") == 0) {
      cmd_bench();
    } else if (strcmp(buf, "uptime") == 0) {
      cmd_uptime();
    } else if (strcmp(buf, "meminfo") == 0) {
      cmd_meminfo();
    } else if (strcmp(buf, "intstats") == 0) {
      cmd_intstats();
    } else if (strncmp(buf, "sleep ", 6) == 0) {
      cmd_sleep(buf + 6);
    } else {
      kprintf("Unknown command: %s\n", buf);
      kprintf("Type 'help' for available commands\n");
    }

    sched_maybe_yield_safe();
  }
}

void shell_task(void *arg) {
  (void)arg;
  shell_run();
}
