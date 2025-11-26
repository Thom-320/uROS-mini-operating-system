#include "config.h"
#include "uros.h"

volatile u64 g_ticks = 0;
volatile int need_resched = 0;

static inline int is_s_timer_interrupt(u64 scause) {
  return (scause >> 63) && ((scause & 0xff) == 5);
}

void trap_handler_c(u64 scause, u64 sepc, u64 stval) {
  (void)sepc;
  (void)stval;

  if (is_s_timer_interrupt(scause)) {
    timer_schedule_next();
    g_ticks++;

    sched_on_tick();
    need_resched = 1;
    return;
  }

  // Unhandled trap
  kprintf("!!! TRAP !!! scause=0x%x sepc=0x%x stval=0x%x\n", scause, sepc,
          stval);
  while (1)
    ;
}

// Minimal assembly wrapper
__asm__(".align 4\n"
        ".global trap_handler\n"
        "trap_handler:\n"
        "addi sp, sp, -128\n"
        "sd ra, 0(sp)\n"
        "sd a0, 8(sp)\n"
        "sd a1, 16(sp)\n"
        "sd a2, 24(sp)\n"
        "sd a3, 32(sp)\n"
        "sd a4, 40(sp)\n"
        "sd a5, 48(sp)\n"
        "sd a6, 56(sp)\n"
        "sd a7, 64(sp)\n"
        "sd t0, 72(sp)\n"
        "sd t1, 80(sp)\n"
        "sd t2, 88(sp)\n"
        "sd t3, 96(sp)\n"
        "sd t4, 104(sp)\n"
        "sd t5, 112(sp)\n"
        "sd t6, 120(sp)\n"

        "csrr a0, scause\n"
        "csrr a1, sepc\n"
        "csrr a2, stval\n"
        "call trap_handler_c\n"

        "ld t6, 120(sp)\n"
        "ld t5, 112(sp)\n"
        "ld t4, 104(sp)\n"
        "ld t3, 96(sp)\n"
        "ld t2, 88(sp)\n"
        "ld t1, 80(sp)\n"
        "ld t0, 72(sp)\n"
        "ld a7, 64(sp)\n"
        "ld a6, 56(sp)\n"
        "ld a5, 48(sp)\n"
        "ld a4, 40(sp)\n"
        "ld a3, 32(sp)\n"
        "ld a2, 24(sp)\n"
        "ld a1, 16(sp)\n"
        "ld a0, 8(sp)\n"
        "ld ra, 0(sp)\n"
        "addi sp, sp, 128\n"
        "sret\n");

void trap_init(void) {
  extern void trap_handler(void);
  u64 handler_addr = (u64)trap_handler;

  // Set stvec (direct mode)
  __asm__ volatile("csrw stvec, %0" ::"r"(handler_addr));

  // Mask timer until the first scheduling tick is programmed
  sbi_set_timer(~0ULL);

  // Enable STIE in sie (bit 5)
  u64 sie_val = (1UL << 5);
  __asm__ volatile("csrs sie, %0" ::"r"(sie_val));

  // NOTE: We do NOT enable global interrupts (sstatus.SIE) here.
  // They will be enabled when the first task is scheduled (sret).
}
