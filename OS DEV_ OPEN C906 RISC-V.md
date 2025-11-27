# OS DEV: OPEN C906 RISC-V

**Authors:**
Maria Paula Chaparro Espinosa
Thomas Chisca Londoño
Simon Velez Castillo

**Institution:**
Universidad Nuestra Señora Mayor del Rosario
Facultad de Ciencia y Tecnología
Matemáticas Aplicadas y Ciencias de la Computación

**Course:** Operating Systems
**Professor:** Pedro Mario Wightman Rojas
**Date:** November 2025

---

## 1. INTRODUCTION

The RISC-V architecture offers an open-source alternative to proprietary Instruction Set Architectures (ISAs) like x86 or ARM. However, the ecosystem for RISC-V operating systems is still maturing. This gap motivated the development of **Helios**, a minimal operating system designed for educational purposes on the RISC-V 64-bit environment.

The primary goal of this project is to demonstrate fundamental operating system concepts through practical implementation. Rather than aiming for production-level features, we focused on creating a clean, understandable codebase that illustrates core OS mechanisms including process scheduling, memory management, hardware abstraction, and system calls.

**Helios** serves as both a learning tool and a research platform. By implementing real OS components from the ground up, we gained deeper insights into the practical considerations of system software development that theoretical study alone cannot provide.

---

## 2. PROJECT VISION & ARCHITECTURE

**Helios** is built around a microkernel-inspired architecture that keeps the core system lean while isolating hardware-specific components. This design facilitates reasoning about the system and allows for future expansion. The kernel is organized into distinct layers:

| Layer                                | Core Function                                          | Components                                                                                                                                                                                                                                                   |
| :----------------------------------- | :----------------------------------------------------- | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Hardware Abstraction Layer (HAL)** | Translates kernel requests into hardware instructions. | • **UART Driver:** NS16550A-compatible serial communication.<br>• **Timer Interface:** Handles hardware interrupts for preemption.<br>• **Memory Mapping:** Basic physical memory handling.                                                                  |
| **Kernel Core**                      | Manages resources, execution, and communication.       | • **Task Management:** Process Control Blocks (PCBs) and context switching.<br>• **Scheduler:** Round-Robin and SJF algorithms.<br>• **Memory Management:** Free-list allocator with coalescing.<br>• **System Calls:** Interface between user/kernel space. |
| **User Interface**                   | Provides interactivity and observability.              | • **Interactive Shell:** Command-line interface.<br>• **System Monitoring:** Tools like `ps` and `meminfo`.                                                                                                                                                  |
| **Development Strategy**             | Ensures reproducibility and stability.                 | • **RISC-V GNU Toolchain:** Cross-compilation.<br>• **QEMU:** System emulation.<br>• **Makefile:** Automated build system.                                                                                                                                   |

---

## 3. CORE SYSTEM IMPLEMENTATION

Our development process followed a structured approach, aligning with the course syllabus to cover key OS topics.

### 3.1 Process Management (Weeks 3-7)

We implemented a functional task system capable of true context switching.

- **Task Control Blocks (TCBs):** We designed a structure to manage process states (NEW, READY, RUNNING, ZOMBIE).
- **Context Switching:** A critical component implemented in assembly (`boot/start.S`) to save and restore CPU registers. _Note: We solved a critical bug where register corruption caused system hangs by reordering the restoration sequence._

**Context Switch Implementation (`boot/start.S`):**

```assembly
# Context switch: ctx_switch(context_t *from, context_t *to)
# a0 = from, a1 = to
ctx_switch:
    # If 'from' (a0) is NULL, skip saving
    beqz a0, 1f

    # Save context to 'from' (a0)
    sd x1,  0(a0)      # ra
    sd x2,  8(a0)      # sp
    # ... (saving all registers x3-x31) ...
    sd x31, 240(a0)    # t6

    # Save CSRs
    csrr t0, sstatus
    sd t0, 248(a0)
    csrr t0, sepc
    sd t0, 256(a0)

1:
    # Restore CSRs first (using t0 as temp)
    ld t0, 248(a1)
    csrw sstatus, t0
    ld t0, 256(a1)
    csrw sepc, t0

    # Restore context from 'to' (a1)
    ld x1,  0(a1)      # ra
    ld x2,  8(a1)      # sp
    # ... (restoring all registers) ...

    ret
```

- **Scheduling:** We implemented a **Round-Robin scheduler** for fairness and a **Shortest Job First (SJF)** scheduler for performance optimization.

**SJF Scheduler Logic (`kernel/sched.c`):**

```c
static pcb_t *sched_pick_next_sjf(void) {
  // ... (find task with minimum burst estimate) ...

  // Remove from queue
  pcb_t *task = ready_queue[best_idx];

  // Shift queue to remove the selected task
  // We only shift elements AFTER the removed task
  for (int i = best_offset; i < ready_count - 1; i++) {
    int dst_idx = (ready_head + i) % MAX_TASKS;
    int src_idx = (ready_head + i + 1) % MAX_TASKS;
    ready_queue[dst_idx] = ready_queue[src_idx];
  }

  ready_count--;
  return task;
}
```

**[INSERT IMAGE HERE: Screenshot of the `bench` command output]**
_Figure 1: Benchmark results comparing Round-Robin and SJF performance._

### 3.2 Synchronization (Weeks 6-10)

To manage concurrency, we implemented **Semaphores** and **Mutexes**. These primitives allow tasks to coordinate and share resources without race conditions.

**Semaphore Implementation (`kernel/sync.c`):**

```c
void sem_wait(sem_t *s) {
    // Cooperative busy-wait
    while (s->count <= 0) {
        // Yield to other tasks while waiting
        sched_maybe_yield_safe();
        task_yield();
    }

    // Critical section: decrement count
    disable_irq();
    s->count--;
    enable_irq();
}

void sem_post(sem_t *s) {
    // Critical section: increment count
    disable_irq();
    s->count++;
    enable_irq();
}
```

- **Producer-Consumer Demo:** We created a specific command (`pcdemo`) to demonstrate these primitives in action.

**[INSERT IMAGE HERE: Screenshot of the `pcdemo` command running]**
_Figure 2: The Producer-Consumer demo illustrating process synchronization._

### 3.3 Memory Management (Weeks 10-14)

We moved beyond simple static allocation to a dynamic memory manager.

- **Free-List Allocator:** We implemented `kmalloc` and `kfree` using a free-list strategy with block coalescing. This reduces fragmentation and allows for efficient memory reuse.

**Memory Allocation Logic (`kernel/kmem.c`):**

```c
// First-fit allocation with splitting
void *kmalloc(size_t size) {
    // ... (alignment and initialization) ...

    // Search for first-fit block
    mem_header_t *current = free_list;

    while (current) {
        if (current->free && current->size >= size) {
            // Found a suitable block

            // Should we split this block?
            size_t remaining = current->size - size;
            if (remaining >= HEADER_SIZE + MIN_ALLOC_SIZE) {
                // Split the block logic...
            }

            // Allocate this block
            current->free = 0;
            return (void *)((u8 *)current + HEADER_SIZE);
        }
        current = current->next;
    }
    return (void *)0;  // Out of memory
}
```

- **Heap Management:** The system manages a dedicated heap space for dynamic data.

**[INSERT IMAGE HERE: Screenshot of the `meminfo` command]**
_Figure 3: Real-time memory usage statistics showing heap allocation._

---

## 4. KEY FEATURES & VISUALS

### 4.1 Boot Sequence & Context Switch

We implemented a complete context switch that preserves all 31 RISC-V general-purpose registers and critical CSRs (sstatus, sepc). This enables true preemptive multitasking.

**[INSERT IMAGE HERE: Screenshot of the boot sequence with the ASCII Art Banner]**
_Figure 4: Helios OS Boot Sequence._

### 4.2 Task Management System

The system supports creating, listing, and killing tasks dynamically.

**[INSERT IMAGE HERE: Diagram or Code Snippet of the `struct process` (PCB)]**
_Figure 5: Process Control Block structure._

### 4.3 Interactive Shell

Unlike many educational OS projects that are static, **Helios** features a fully interactive shell with over 10 commands.

- **Commands:** `help`, `ps` (process list), `kill` (terminate process), `sched` (switch scheduler), `bench` (benchmark), `meminfo` (memory stats).

**[INSERT IMAGE HERE: Screenshot of the `help` command output]**
_Figure 6: Interactive Shell Interface._

---

## 5. CONCLUSION

The **Helios** project successfully demonstrates the implementation of a functional operating system kernel for RISC-V. We have achieved:

1.  **Stability:** A robust kernel that handles interrupts and context switches without crashing.
2.  **Functionality:** A rich set of features including multitasking, synchronization, and dynamic memory.
3.  **Interactivity:** A user-friendly shell that allows for real-time system inspection.

This project bridges the gap between OS theory and practice, providing a solid foundation for future exploration into virtual memory, file systems, and user-mode applications.

---

## 6. REFERENCES

1.  _Operating Systems: Three Easy Pieces_ - Remzi H. Arpaci-Dusseau and Andrea C. Arpaci-Dusseau.
2.  _The RISC-V Reader: An Open Architecture Atlas_ - David Patterson and Andrew Waterman.
3.  _OSDev.org Wiki_ - Various Contributors.
