<!-- 1385f545-517d-450c-a7c8-44bb3f38e523 5ba95bfc-3710-4aa8-830e-0628fb274a0f -->
# uROS RISC-V Mini-OS Implementation

## Architecture Overview

- **Platform**: QEMU virt, RISC-V 64 (rv64gc), S-mode bare metal
- **Base address**: 0x80200000 (linker script)
- **Boot**: OpenSBI (-bios default)
- **Timer**: SBI set_timer, 100 Hz tickrate (~10ms)
- **Scheduler**: RR preemptive (quantum=5 ticks) + SJF non-preemptive
- **Tasks**: Kernel threads with context switching

## File Structure

```
/Users/thom/Library/Mobile Documents/com~apple~CloudDocs/Universidad/OS/mini-os/
├── Makefile
├── linker.ld
├── boot/start.S
├── kernel/kmain.c
├── kernel/trap.c
├── kernel/task.c
├── kernel/sched.c
├── kernel/shell.c
├── drivers/uart.c
├── drivers/timer.c
├── lib/printf.c
├── include/uros.h
├── scripts/run-qemu.sh
├── scripts/demo.sh
└── docs/README.md
```

## Implementation Steps

### 1. Core Infrastructure & Headers

**File: `include/uros.h`**

- Define basic types (u8, u16, u32, u64, size_t)
- Define `context_t` struct (x1-x31, sstatus, sepc)
- Define `pcb_t` struct with: pid, state (NEW/READY/RUNNING/SLEEPING/ZOMBIE), context, stack pointer, entry function, ticks_used, burst_hint, burst_estimate, arrival_time, finish_time, wait_time
- Prototypes for all subsystems: uart, timer, trap, task, sched, shell, printf
- SBI call interface prototype
- Constants: UART_BASE=0x10000000, TICK_HZ=100, RR_QUANTUM=5

### 2. Linker Script & Boot Assembly

**File: `linker.ld`**

- BASE = 0x80200000
- Sections: .text, .rodata, .data, .bss
- PROVIDE(_stack_top) for main kernel stack

**File: `boot/start.S`**

- Entry point: set sp to _stack_top
- Jump to kmain
- **ctx_switch(from, to)** implementation:
  - Save x1-x31, sstatus, sepc to from->context
  - Restore x1-x31, sstatus, sepc from to->context
  - Use register pairs: sd/ld with offset calculations
  - Handle sp specially (x2)

### 3. UART Driver (NS16550A)

**File: `drivers/uart.c`**

- MMIO base 0x10000000
- Registers: THR (0), RBR (0), LSR (5), IER (1), LCR (3)
- `uart_init()`: configure 8N1, disable interrupts
- `uart_putc(char c)`: wait for THRE bit in LSR, write to THR
- `uart_getc()`: poll RBR with timeout
- `uart_puts(const char *s)`: loop uart_putc
- Input buffering with echo and backspace support

### 4. Printf Implementation

**File: `lib/printf.c`**

- `kprintf(const char *fmt, ...)` with va_args
- Support: %s, %d, %u, %x, %c
- Use uart_putc for output
- Simple number-to-string conversion helpers

### 5. Timer Driver (SBI)

**File: `drivers/timer.c`**

- `sbi_call(eid, fid, a0-a5)`: inline assembly for ecall
- `sbi_set_timer(u64 stime_value)`: EID=0, FID=0
- `rdtime()`: read CSR time with inline asm
- `timer_init(hz)`: calculate tick_delta = TIMEBASE_FREQ / hz (assume 10MHz)
- `timer_schedule_next()`: rdtime() + tick_delta, call sbi_set_timer
- Global g_ticks counter

### 6. Trap Handling

**File: `kernel/trap.c`**

- `trap_init()`:
  - Set stvec to trap_handler address (direct mode)
  - Enable SIE bit in sstatus: `sstatus |= (1 << 1)`
  - Enable STIE bit in sie: `sie |= (1 << 5)`
- `trap_handler()`: read scause
  - If supervisor timer interrupt (bit 63 set, code 5):
    - Call timer_schedule_next()
    - Increment g_ticks
    - Call sched_on_tick()
  - Handle other traps (print and halt)

### 7. Task Management

**File: `kernel/task.c`**

- Bump allocator: static heap array, `kmalloc(size)` advances pointer
- Task table: MAX_TASKS=32, array of pcb_t
- `task_create(void (*entry)(void*), void *arg, int burst_hint)`:
  - Allocate PID, stack (4KB per task)
  - Initialize context: set sepc to entry, sp to stack_top
  - Set burst_hint and burst_estimate
  - Set arrival_time = g_ticks
  - State = NEW → READY, add to scheduler
- `task_exit()`: mark ZOMBIE, yield
- `yield()`: call sched_yield() which does ctx_switch
- `task_get_by_pid(pid)`: lookup helper
- Track memory usage for meminfo

### 8. Scheduler (RR + SJF)

**File: `kernel/sched.c`**

- Mode enum: SCHED_RR, SCHED_SJF
- Global: current_mode, current_task, ready_queue
- **Round-Robin (preemptive)**:
  - `sched_on_tick()`: if mode==RR, decrement quantum; if 0, yield
  - `sched_pick_next()`: FIFO from ready_queue
- **SJF (non-preemptive)**:
  - `sched_pick_next()`: sort/find task with min burst_estimate
  - Tie-break: arrival_time (FCFS) or PID
  - Update estimate after task completes: τ_new = 0.5 * real + 0.5 * τ_old
- `sched_set_mode(mode)`: switch between RR/SJF
- `sched_yield()`: ctx_switch to next task
- Track wait_time, finish_time for metrics

### 9. Shell & Commands

**File: `kernel/shell.c`**

- `shell_run()`: infinite loop reading lines from UART
- Parse command and dispatch:
  - **help**: list all commands
  - **ps**: print PID, state, ticks_used, burst_estimate for all tasks
  - **run cpu**: create CPU-bound task (loop with occasional yield)
  - **run io**: create I/O task (sleep 5 ticks between prints)
  - **kill \<pid\>**: mark task as ZOMBIE
  - **sched rr|sjf**: call sched_set_mode()
  - **bench**: run benchmark (see below)
  - **uptime**: print g_ticks / 100.0 seconds
  - **meminfo**: print bump allocator usage, stack memory

**Benchmark implementation**:

- Create 6-10 tasks with known burst_hints (e.g., 10, 20, 15, 30, 25, 12 ticks)
- **Round 1 (RR)**:
  - Set mode to RR
  - Create tasks, wait until all ZOMBIE
  - Collect: wait_time, turnaround (finish - arrival), total duration
- **Round 2 (SJF)**:
  - Reset task table
  - Set mode to SJF
  - Create same tasks (reset arrival times)
  - Wait until all ZOMBIE
  - Collect same metrics
- Print comparison table:
  ```
  Benchmark Results:
            RR      SJF
  Wait:     X.XX    Y.YY ticks
  T.around: X.XX    Y.YY ticks
  T.put:    X.XX    Y.YY tasks/sec
  ```


### 10. Kernel Main

**File: `kernel/kmain.c`**

- Call uart_init()
- Call trap_init()
- Call timer_init(100)
- Call sched_init()
- Print banner: "uROS (rv64gc, QEMU virt) - console ready\nticks=0\n"
- Create 1-2 demo tasks (optional)
- Call shell_run() (never returns)

### 11. Build System

**File: `Makefile`**

- Toolchain: CROSS=riscv64-unknown-elf-
- CFLAGS: -march=rv64gc -mabi=lp64 -ffreestanding -nostdlib -nostartfiles -Wall -Wextra -O2 -I include
- LDFLAGS: -T linker.ld -nostdlib
- Targets:
  - **all**: compile all .c and .S files, link to build/kernel.elf, generate kernel.map
  - **run**: execute scripts/run-qemu.sh
  - **clean**: rm -rf build/
  - **dtb**: dump QEMU DTB and convert to DTS with dtc

**File: `scripts/run-qemu.sh`**

```bash
#!/bin/bash
qemu-system-riscv64 -machine virt -nographic -bios default -kernel build/kernel.elf
```

**File: `scripts/demo.sh`**

```bash
#!/bin/bash
(sleep 1; echo "help"; sleep 1; echo "ps"; sleep 1; 
 echo "run cpu"; sleep 1; echo "run io"; sleep 1; 
 echo "sched rr"; sleep 1; echo "bench") | ./scripts/run-qemu.sh
```

### 12. Documentation

**File: `docs/README.md`**

- Requirements: QEMU, riscv64-unknown-elf toolchain
- Build steps: make, make run
- Shell commands reference
- RR vs SJF explanation with metrics definitions
- Example output log

## Definition of Done

- `make run` boots to "uROS ready" banner
- Interactive prompt "uROS> " accepts commands
- `help` lists all commands
- `run cpu` and `run io` create visible tasks
- `ps` shows task states and metrics
- `sched rr` and `sched sjf` switch modes
- `bench` prints comparison table with wait, turnaround, throughput
- `uptime` and `meminfo` work correctly
- Tasks preempt visibly under RR
- Context switch preserves registers correctly

### To-dos

- [ ] Create include/uros.h with types, structs (context_t, pcb_t), and all function prototypes
- [ ] Implement linker.ld with BASE=0x80200000, sections, and stack
- [ ] Implement boot/start.S with entry point and ctx_switch assembly routine
- [ ] Implement drivers/uart.c for NS16550A at 0x10000000
- [ ] Implement lib/printf.c with kprintf supporting %s %d %u %x
- [ ] Implement drivers/timer.c with SBI calls and 100Hz tick scheduling
- [ ] Implement kernel/trap.c with stvec setup, SIE/STIE enable, and timer interrupt handler
- [ ] Implement kernel/task.c with bump allocator, task_create, yield, exit
- [ ] Implement kernel/sched.c with RR preemptive and SJF non-preemptive modes
- [ ] Implement kernel/shell.c with all commands including bench with RR vs SJF comparison
- [ ] Implement kernel/kmain.c with initialization sequence and banner
- [ ] Create Makefile with all/run/clean/dtb targets
- [ ] Create scripts/run-qemu.sh and scripts/demo.sh
- [ ] Create docs/README.md with requirements, usage, and explanation