#include "semaphore.h"

void sem_init(semaphore_t *sem, int initial_val) {
    sem->value = initial_val;
    sem->wait_queue = 0;
}

void sem_wait(semaphore_t *sem) {
    __asm__ volatile("cli");
    sem->value--;
    if (sem->value < 0) {
        thread_t *curr = thread_current();
        curr->next_blocked = sem->wait_queue;
        sem->wait_queue = curr;
        thread_block();
    }
    __asm__ volatile("sti");
}

void sem_post(semaphore_t *sem) {
    __asm__ volatile("cli");
    sem->value++;
    if (sem->value <= 0 && sem->wait_queue) {
        thread_t *wakeup = sem->wait_queue;
        sem->wait_queue = wakeup->next_blocked;
        wakeup->next_blocked = 0;
        thread_unblock(wakeup);
    }
    __asm__ volatile("sti");
}
