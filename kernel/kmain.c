#include "uros.h"

void kmain(void) {
  uart_init();

  kprintf("\n");
  kprintf("  _   _      _ _  ___  ____  \n");
  kprintf(" | | | | ___| (_)/ _ \\/ ___| \n");
  kprintf(" | |_| |/ _ \\ | | | | \\___ \\ \n");
  kprintf(" |  _  |  __/ | | |_| |___) |\n");
  kprintf(" |_| |_|\\___|_|_|\\___/|____/ \n");
  kprintf("                             \n");
  kprintf(" HeliOS v1.0 - RISC-V 64-bit \n");
  kprintf("--------------------------------------------------\n");

  kprintf("Initializing task system...\n");
  task_init();

  kprintf("Initializing scheduler...\n");
  sched_init();

  kprintf("Creating idle task...\n");
  if (task_create((void (*)(void *))idle_task, (void *)0, 10000) < 0) {
    kprintf("Failed to create idle task\n");
    for (;;) {
      __asm__ volatile("wfi");
    }
  }

  kprintf("Creating shell task...\n");
  if (task_create(shell_task, (void *)0, 1000) < 0) {
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
