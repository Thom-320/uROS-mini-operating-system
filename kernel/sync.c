#include "uros.h"

// Semaphore implementation with cooperative busy-wait
void sem_init(sem_t *s, int count) {
    s->count = count;
}

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

// Mutex implementation (binary semaphore wrapper)
void mutex_init(mutex_t *m) {
    sem_init(&m->s, 1);  // Binary semaphore (0 or 1)
}

void mutex_lock(mutex_t *m) {
    sem_wait(&m->s);
}

void mutex_unlock(mutex_t *m) {
    sem_post(&m->s);
}

