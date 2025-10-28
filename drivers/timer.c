#include "uros.h"

// SBI timer extension ID and function ID
#define SBI_EID_TIME 0x54494D45
#define SBI_FID_SET_TIMER 0

#define TIMER_TIMEBASE_HZ 10000000UL
#define TIMER_DELTA_CYCLES (TIMER_TIMEBASE_HZ / TICK_HZ)

static u64 tick_delta = TIMER_DELTA_CYCLES;

// SBI call interface
static inline long sbi_call(long eid, long fid, 
                           long a0, long a1, long a2,
                           long a3, long a4, long a5) {
    register long r_eid __asm__("a7") = eid;
    register long r_fid __asm__("a6") = fid;
    register long r_a0 __asm__("a0") = a0;
    register long r_a1 __asm__("a1") = a1;
    register long r_a2 __asm__("a2") = a2;
    register long r_a3 __asm__("a3") = a3;
    register long r_a4 __asm__("a4") = a4;
    register long r_a5 __asm__("a5") = a5;
    
    __asm__ volatile (
        "ecall"
        : "+r"(r_a0), "+r"(r_a1)
        : "r"(r_a0), "r"(r_a1), "r"(r_a2), "r"(r_a3), "r"(r_a4), "r"(r_a5),
          "r"(r_fid), "r"(r_eid)
        : "memory"
    );
    
    return r_a0;
}

void sbi_set_timer(u64 stime_value) {
    sbi_call(SBI_EID_TIME, SBI_FID_SET_TIMER, stime_value, 0, 0, 0, 0, 0);
}

u64 rdtime(void) {
    u64 time;
    __asm__ volatile("csrr %0, time" : "=r"(time));
    return time;
}

void timer_init(void) {
    tick_delta = TIMER_DELTA_CYCLES;
    timer_schedule_next();
}

void timer_schedule_next(void) {
    if (tick_delta == 0) {
        tick_delta = TIMER_DELTA_CYCLES;
    }
    u64 next = rdtime() + tick_delta;
    sbi_set_timer(next);
}
