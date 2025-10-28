#ifndef UROS_H
#define UROS_H

#include "config.h"

// Basic types
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;
typedef unsigned long size_t;

// Constants
#define UART_BASE       0x10000000
#define TICK_HZ         100
#define RR_QUANTUM      5
#define STACK_SIZE      8192
#define MAX_TASKS       32
#define HEAP_SIZE       (256 * 1024)

// Task states
typedef enum {
    TASK_NEW = 0,
    TASK_READY,
    TASK_RUNNING,
    TASK_SLEEPING,
    TASK_ZOMBIE
} task_state_t;

// Scheduler modes
typedef enum {
    SCHED_RR = 0,
    SCHED_SJF
} sched_mode_t;

// Context structure - offsets must match boot/start.S exactly
// Total size: 34 * 8 = 272 bytes
typedef struct {
    u64 x1;   // ra    offset 0
    u64 x2;   // sp    offset 8
    u64 x3;   // gp    offset 16
    u64 x4;   // tp    offset 24
    u64 x5;   // t0    offset 32
    u64 x6;   // t1    offset 40
    u64 x7;   // t2    offset 48
    u64 x8;   // s0/fp offset 56
    u64 x9;   // s1    offset 64
    u64 x10;  // a0    offset 72
    u64 x11;  // a1    offset 80
    u64 x12;  // a2    offset 88
    u64 x13;  // a3    offset 96
    u64 x14;  // a4    offset 104
    u64 x15;  // a5    offset 112
    u64 x16;  // a6    offset 120
    u64 x17;  // a7    offset 128
    u64 x18;  // s2    offset 136
    u64 x19;  // s3    offset 144
    u64 x20;  // s4    offset 152
    u64 x21;  // s5    offset 160
    u64 x22;  // s6    offset 168
    u64 x23;  // s7    offset 176
    u64 x24;  // s8    offset 184
    u64 x25;  // s9    offset 192
    u64 x26;  // s10   offset 200
    u64 x27;  // s11   offset 208
    u64 x28;  // t3    offset 216
    u64 x29;  // t4    offset 224
    u64 x30;  // t5    offset 232
    u64 x31;  // t6    offset 240
    u64 sstatus;      // offset 248
    u64 sepc;         // offset 256
} context_t;

// Process Control Block
typedef struct {
    int pid;
    task_state_t state;
    context_t context;
    void *stack_base;
    void (*entry)(void *);
    void *arg;
    u64 ticks_used;
    int burst_hint;
    u64 burst_estimate;
    u64 arrival_time;
    u64 start_time;
    u64 finish_time;
    u64 wait_time;
} pcb_t;

// Global tick counter
extern volatile u64 g_ticks;
extern volatile int need_resched;

// IRQ helpers
static inline void disable_irq(void) {
    __asm__ volatile("csrci sstatus, 2"); // clear SIE bit (bit 1)
}

static inline void enable_irq(void) {
    __asm__ volatile("csrsi sstatus, 2"); // set SIE bit (bit 1)
}

// UART functions
void uart_init(void);
void uart_putc(char c);
int uart_getc(void);
int uart_getc_blocking(void);
void uart_puts(const char *s);
char *uart_gets(char *buf, int maxlen);

// Printf
void kprintf(const char *fmt, ...);

// Timer functions
void timer_init(void);
void timer_schedule_next(void);
u64 rdtime(void);
void sbi_set_timer(u64 stime_value);

// Trap functions
void trap_init(void);
void trap_handler(void);

// Task functions
void task_init(void);
int task_create(void (*entry)(void *), void *arg, int burst_hint);
void task_exit(void);
void task_yield(void);
pcb_t *task_get_by_pid(int pid);
void task_reap(int pid);
void *kmalloc(size_t size);
void kfree(void *ptr);
size_t kmalloc_used(void);
size_t kmalloc_free(void);
int kmalloc_free_blocks(void);
void idle_task(void *arg);

// Scheduler functions
void sched_init(void);
void sched_yield(void);
void sched_on_tick(void);
void sched_set_mode(sched_mode_t mode);
sched_mode_t sched_get_mode(void);
void sched_add_ready(pcb_t *task);
pcb_t *sched_current(void);
void sched_maybe_yield_safe(void);
void sched_set_preempt(int on);
int sched_get_preempt(void);

// Shell functions
void shell_run(void);
void shell_task(void *arg);

// Synchronization primitives
typedef struct {
    volatile int count;
} sem_t;

void sem_init(sem_t *s, int count);
void sem_wait(sem_t *s);
void sem_post(sem_t *s);

typedef struct {
    sem_t s;
} mutex_t;

void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);

// Context switch (ASM)
void ctx_switch(context_t *from, context_t *to);

// Utility
void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strlen(const char *s);
int atoi(const char *s);

#endif // UROS_H
