#include "mutex.h"

void mutex_init(mutex_t *m) {
    m->locked = 0;
    m->owner = 0;
    m->wait_queue = 0;
}

void mutex_lock(mutex_t *m) {
    while (1) {
        __asm__ volatile("cli");
        if (!m->locked) {
            m->locked = 1;
            m->owner = thread_current();
            __asm__ volatile("sti");
            return;
        }

        // Add current thread to mutex wait queue
        thread_t *curr = thread_current();
        curr->next_blocked = m->wait_queue;
        m->wait_queue = curr;

        // Block and switch out (thread_block re-enables interrupts in scheduler)
        thread_block();
        __asm__ volatile("sti");
    }
}

void mutex_unlock(mutex_t *m) {
    __asm__ volatile("cli");
    if (!m->locked) {
        __asm__ volatile("sti");
        return;
    }

    m->locked = 0;
    m->owner = 0;

    // Wake up the head of the wait queue
    if (m->wait_queue) {
        thread_t *wakeup = m->wait_queue;
        m->wait_queue = wakeup->next_blocked;
        wakeup->next_blocked = 0;
        thread_unblock(wakeup);
    }
    __asm__ volatile("sti");
}
