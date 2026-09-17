#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "thread.h"

typedef struct {
    volatile int value;
    thread_t *wait_queue;
} semaphore_t;

void sem_init(semaphore_t *sem, int initial_val);
void sem_wait(semaphore_t *sem);
void sem_post(semaphore_t *sem);

#endif
