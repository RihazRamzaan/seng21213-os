#include "scheduler.h"
#include "process.h"

extern process_t process_table[MAX_PROCESSES];
extern process_t *current_process;

static int current_index = 0;

void scheduler_init(void) {
    current_index = 0;
}

void schedule(void) {
    if (!current_process) return;

    int next_index = -1;
    for (int i = 1; i <= MAX_PROCESSES; i++) {
        int idx = (current_index + i) % MAX_PROCESSES;
        if (process_table[idx].state == PROCESS_READY) {
            next_index = idx;
            break;
        }
    }

    if (next_index == -1) {
        if (current_process->state == PROCESS_RUNNING) return;
        next_index = 0; // Shell
    }

    if (next_index == current_index && current_process->state == PROCESS_RUNNING) {
        return;
    }

    process_t *old = current_process;
    process_t *next = &process_table[next_index];

    if (old->state == PROCESS_RUNNING) {
        old->state = PROCESS_READY;
    }
    next->state = PROCESS_RUNNING;
    current_process = next;
    current_index = next_index;
}