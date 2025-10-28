#include "uros.h"

void kmain(void) {
    uart_init();

    kprintf("uROS (rv64gc, QEMU virt) - console ready\n");
    kprintf("ticks=0\n");

    kprintf("Initializing task system...\n");
    task_init();

    kprintf("Initializing scheduler...\n");
    sched_init();

    kprintf("Creating idle task...\n");
    if (task_create((void (*)(void *))idle_task, (void *)0, 1) < 0) {
        kprintf("Failed to create idle task\n");
        for (;;) {
            __asm__ volatile("wfi");
        }
    }

    kprintf("Creating shell task...\n");
    if (task_create(shell_task, (void *)0, 10) < 0) {
        kprintf("Failed to create shell task\n");
        for (;;) {
            __asm__ volatile("wfi");
        }
    }

    kprintf("Initializing trap handling...\n");
    trap_init();

    kprintf("Initializing timer...\n");
    timer_init();

    kprintf("System initialized, starting scheduler...\n");

    need_resched = 1;
    sched_yield();

    for (;;) {
        __asm__ volatile("wfi");
    }
}
