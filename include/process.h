#ifndef PROCESS_H
#define PROCESS_H

#include "../include/types.h"

#define MAX_PROCESSES 16
#define STACK_SIZE    4096

typedef enum {
    PROCESS_UNUSED = 0,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

typedef struct process {
    int pid;
    char name[32];
    uint32_t esp;
    uint8_t stack[STACK_SIZE];
    process_state_t state;
} process_t;

void process_init(void);
int  process_create(const char *name, void (*entry_point)(void));
void process_kill(int pid);
process_t *process_get_current(void);
void process_list(void);

#endif
