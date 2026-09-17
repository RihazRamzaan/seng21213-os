#ifndef THREAD_H
#define THREAD_H

#include "../include/types.h"
#include "process.h"

#define MAX_THREADS 16

typedef enum {
    THREAD_UNUSED = 0,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_TERMINATED
} thread_state_t;

typedef struct thread {
    int tid;
    char name[32];
    uint32_t esp;
    uint8_t stack[STACK_SIZE];
    thread_state_t state;
    struct thread *next_blocked; // Linked list for mutex/semaphore wait queues
} thread_t;

void thread_init(void);
int  thread_create(const char *name, void (*entry_point)(void));
void thread_exit(void);
void thread_block(void);
void thread_unblock(thread_t *t);
thread_t *thread_current(void);
void thread_list(void);

#endif
