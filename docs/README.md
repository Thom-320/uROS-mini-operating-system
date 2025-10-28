# uROS - RISC-V Mini Operating System

A minimal operating system for RISC-V 64-bit architecture running on QEMU virt platform.

## Features

- **Platform**: QEMU virt, RISC-V 64 (rv64gc), S-mode bare metal
- **Boot**: OpenSBI firmware (`-bios default`)
- **Console**: NS16550A UART driver with interactive shell
- **Timer**: SBI-based timer with 100 Hz tick rate (~10ms)
- **Tasks**: Kernel threads with context switching
- **Scheduling**: 
  - Round-Robin (RR) - Preemptive with 5-tick quantum
  - Shortest Job First (SJF) - Non-preemptive with exponential burst estimation
- **Benchmarking**: Compare RR vs SJF performance metrics

## Requirements

- QEMU 7.0+ with RISC-V support
- `riscv64-unknown-elf-gcc` toolchain
- `dtc` (Device Tree Compiler) for DTB inspection

### Installing Requirements (macOS)

```bash
brew install qemu
brew install riscv-gnu-toolchain
brew install dtc
```

### Installing Requirements (Linux)

```bash
# Ubuntu/Debian
sudo apt-get install qemu-system-misc gcc-riscv64-unknown-elf device-tree-compiler

# Arch
sudo pacman -S qemu-system-riscv dtc
yay -S riscv64-elf-gcc
```

## Building and Running

```bash
# Build the kernel
make

# Run in QEMU
make run

# Clean build artifacts
make clean

# Dump device tree blob (for debugging)
make dtb
```

## Shell Commands

Once booted, the system presents an interactive shell prompt: `uROS>`

### Available Commands

- **help** - Display list of available commands
- **ps** - List all tasks with PID, state, CPU ticks used, and burst estimate
- **run cpu** - Create a CPU-bound task (burns CPU cycles)
- **run io** - Create an I/O-bound task (simulates I/O with sleeps)
- **kill \<pid\>** - Terminate task with given PID
- **sched rr** - Switch to Round-Robin scheduler
- **sched sjf** - Switch to Shortest Job First scheduler
- **bench** - Run scheduling benchmark and compare RR vs SJF
- **uptime** - Display system uptime in seconds and ticks
- **meminfo** - Show kernel heap memory usage

## Scheduling Algorithms

### Round-Robin (RR)

- **Type**: Preemptive
- **Quantum**: 5 ticks (~50ms at 100 Hz)
- **Behavior**: Each task runs for its quantum, then the scheduler switches to the next ready task in FIFO order
- **Use Case**: Fair CPU time distribution, good for interactive workloads

### Shortest Job First (SJF)

- **Type**: Non-preemptive
- **Selection**: Task with smallest estimated burst time runs to completion
- **Estimation**: Exponential averaging with α=0.5
  - τ_new = 0.5 × (actual_burst) + 0.5 × τ_old
- **Tie-breaking**: Arrival time (FCFS), then PID
- **Use Case**: Minimizes average wait time when burst times are known/predictable

## Benchmark Metrics

The `bench` command runs two rounds of the same task set:
1. First under Round-Robin
2. Then under SJF

**Metrics reported**:

- **Wait time (avg)**: Average time tasks spend in ready queue before first execution
  - Formula: Σ(start_time - arrival_time) / N
  
- **Turnaround time (avg)**: Average total time from arrival to completion
  - Formula: Σ(finish_time - arrival_time) / N
  
- **Throughput**: Tasks completed per second
  - Formula: N / total_duration

## Synchronization

uROS provides basic synchronization primitives for coordinating concurrent tasks:

### Semaphores

Counting semaphores with cooperative busy-wait implementation:

```c
sem_t sem;
sem_init(&sem, initial_count);  // Initialize with count
sem_wait(&sem);                 // Decrement (wait if ≤0)
sem_post(&sem);                 // Increment
```

**Implementation notes:**
- Uses cooperative busy-wait with `task_yield()` while waiting
- Safe in both preemptive and cooperative modes
- Critical sections protected with `disable_irq()`/`enable_irq()`

### Mutexes

Binary semaphores for mutual exclusion:

```c
mutex_t mutex;
mutex_init(&mutex);    // Initialize
mutex_lock(&mutex);    // Acquire (wait if locked)
mutex_unlock(&mutex);  // Release
```

### Producer-Consumer Demo

The `pcdemo` command demonstrates classic producer-consumer synchronization:

**Buffer:** 16-item circular buffer
**Semaphores:**
- `empty` - tracks available slots (initially 16)
- `full` - tracks filled slots (initially 0)
- `mutex` - protects buffer access

**Algorithm:**
```
Producer:
  sem_wait(&empty)      // Wait for empty slot
  mutex_lock(&mutex)    // Enter critical section
  [produce item]
  mutex_unlock(&mutex)  // Exit critical section
  sem_post(&full)       // Signal item available

Consumer:
  sem_wait(&full)       // Wait for item
  mutex_lock(&mutex)    // Enter critical section
  [consume item]
  mutex_unlock(&mutex)  // Exit critical section
  sem_post(&empty)      // Signal slot available
```

**Running the demo:**
```bash
uROS> pcdemo
=== Producer-Consumer Demo ===
Buffer size: 16 items
Creating producer and consumer tasks...
Producer task created (PID 1)
Consumer task created (PID 2)
Demo running... (will produce/consume 10 items)
Watch the alternating output!

Producer: produced item 1 at index 0
Consumer: consumed item 1 from index 0
Producer: produced item 2 at index 1
Consumer: consumed item 2 from index 1
...
Producer: finished producing 10 items
Consumer: finished consuming 10 items
```

**Observations:**
- **Preemptive mode** (`sched preempt on`): More interleaved execution, smoother alternation
- **Cooperative mode** (`sched preempt off`): Works correctly, alternates at yield points
- **No race conditions**: Mutex ensures buffer integrity
- **No deadlock**: Proper semaphore ordering prevents deadlock
- **Visible delays**: 5-tick sleeps make synchronization observable

## Memory Management

uROS uses a **free-list allocator** with first-fit allocation and coalescing for efficient memory reuse.

### Algorithm: First-Fit with Coalescing

**Data Structure:**
```c
struct mem_header {
    size_t size;      // Block size (excluding header)
    int free;         // 1 = free, 0 = allocated
    struct mem_header *next;  // Next block in list
};
```

**Allocation (`kmalloc`):**
1. Search free list for first block with `size >= requested`
2. If block is large enough, split it:
   - Allocate requested size from beginning
   - Create new free block with remainder
3. Mark block as allocated
4. Return pointer (after header)

**Deallocation (`kfree`):**
1. Mark block as free
2. **Coalesce with next** block if it's free
3. **Coalesce with previous** block if it's free
4. Reduces fragmentation

**Memory Stats:**
```bash
uROS> meminfo
=== Memory Usage ===
Heap total:    262144 bytes
Heap used:     24576 bytes (9%)
Heap free:     237568 bytes (90%)
Free blocks:   5
Fragmentation: moderate
```

### Features

- ✅ **First-fit**: Fast allocation (O(n) where n = number of blocks)
- ✅ **Splitting**: Minimizes waste for small allocations
- ✅ **Coalescing**: Merges adjacent free blocks to prevent fragmentation
- ✅ **Aligned**: All allocations aligned to 16 bytes
- ✅ **Thread-safe**: Uses `disable_irq()`/`enable_irq()`

### Limitations

- ❌ **No best-fit or worst-fit**: Only first-fit implemented
- ❌ **Linear search**: O(n) allocation time
- ❌ **No compaction**: Fragmentation can occur over time
- ❌ **Fixed heap size**: 256KB total

### Use Cases

**Task stacks:** Each task allocates 4KB stack on creation
```c
void *stack = kmalloc(STACK_SIZE);  // 4096 bytes
// ... use stack ...
kfree(stack);  // When task exits
```

**Dynamic buffers:** Shell and synchronization structures
```c
int *buffer = kmalloc(16 * sizeof(int));
// ... use buffer ...
kfree(buffer);
```

### Observing Memory Usage

```bash
# Before creating tasks
uROS> meminfo
Heap used: 8192 bytes

# After pcdemo
uROS> pcdemo
...
uROS> meminfo
Heap used: 24576 bytes  # Two 4KB stacks + buffers

# After tasks finish (become ZOMBIE)
uROS> meminfo
Heap used: 24576 bytes  # Stacks still allocated

# After killing tasks (with task_reap)
uROS> kill 1
uROS> kill 2
uROS> meminfo
Heap used: 8192 bytes  # Stacks freed!
```

### Implementation Notes

- **Header overhead**: 24 bytes per block
- **Minimum split size**: 16 bytes (prevents tiny fragments)
- **Coalescing**: Both forward and backward
- **Initial state**: One large free block (256KB - header)

## Example Session

```
uROS (rv64gc, QEMU virt) - console ready
ticks=0
uROS> help
Available commands:
  help          - Show this help
  ps            - List tasks
  run cpu       - Create CPU-bound task
  run io        - Create I/O-bound task
  kill <pid>    - Kill a task
  sched rr      - Switch to Round-Robin
  sched sjf     - Switch to SJF
  bench         - Run scheduler benchmark
  uptime        - Show system uptime
  meminfo       - Show memory usage

uROS> sched rr
Scheduler: Round-Robin (quantum=5 ticks)

uROS> run cpu
Created CPU task with PID 0

uROS> run io
Created I/O task with PID 1

uROS> ps
PID  STATE     TICKS  BURST_EST  ARRIVAL
0    RUNNING   23     20         10
1    READY     8      15         15

uROS> bench
Running benchmark...
Round 1: Round-Robin...
RR done in 142 ticks
Round 2: SJF...
SJF done in 125 ticks

Benchmark Results (6 tasks):
                  RR        SJF
Wait (avg):       45        28 ticks
Turnaround (avg): 68        48 ticks
Throughput:       4.22      4.80 tasks/sec

uROS> uptime
Uptime: 32.45 seconds (3245 ticks)
```

## Architecture Details

### Memory Layout

- **Base Address**: 0x80200000
- **Kernel Stack**: 16 KB
- **Task Stacks**: 8 KB each (up to 32 tasks)
- **Heap**: 256 KB (bump allocator)

### Context Switching

All general-purpose registers (x1-x31), `sstatus`, and `sepc` are saved/restored on context switch. Stack pointer is aligned to 16 bytes per RISC-V ABI requirements.

### Interrupt Handling

- Timer interrupts occur every 10ms (100 Hz)
- Interrupts are disabled during critical sections (queue/context manipulation)
- Minimal work in IRQ handler - just schedule next tick and set flags

### Task States

Tasks transition through these states:
- **NEW** → **READY** → **RUNNING** → **SLEEPING**/back to **READY** → **ZOMBIE**

## Development

The codebase is organized as follows:

```
uROS/
├── include/uros.h       # Main header with types and prototypes
├── linker.ld            # Linker script
├── boot/start.S         # Boot assembly and context switch
├── kernel/
│   ├── kmain.c          # Kernel entry point
│   ├── trap.c           # Trap/interrupt handling
│   ├── task.c           # Task management and memory allocation
│   ├── sched.c          # Scheduler (RR and SJF)
│   └── shell.c          # Interactive shell
├── drivers/
│   ├── uart.c           # NS16550A UART driver
│   └── timer.c          # SBI timer driver
├── lib/
│   └── printf.c         # Minimal printf and string utilities
└── scripts/
    ├── run-qemu.sh      # QEMU launch script
    └── demo.sh          # Automated demo script
```

## Limitations

- No MMU/paging (bare metal S-mode)
- No user mode (all tasks run in kernel mode)
- Simple bump allocator (no free/realloc)
- No file system or persistent storage
- Single CPU core only

## Future Work

- Add user mode with syscalls
- Implement basic paging/virtual memory
- Add process synchronization primitives (semaphores, mutexes)
- Multi-level feedback queue scheduling
- Real I/O device drivers (block devices, network)

## License

Educational project for OS course.

## Authors

- Simón - Núcleo (Bootstrap & traps)
- María Paula - Drivers/Sistema (UART, shell, build)
- Thomas - Planificación (RR/SJF + métricas)

