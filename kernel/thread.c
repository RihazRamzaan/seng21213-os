#include "thread.h"
#include "vga.h"
#include "scheduler.h"

static thread_t thread_table[MAX_THREADS];
static int next_tid = 1;
thread_t *current_thread = 0;

static void k_strncpy(char *dest, const char *src, size_t n) {
    while (n && *src) { *dest++ = *src++; n--; }
    while (n) { *dest++ = '\0'; n--; }
}

static void int_to_str(int n, char *buf) {
    if (n == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    char temp[12]; int i = 0, j = 0;
    while (n > 0) { temp[i++] = (n % 10) + '0'; n /= 10; }
    while (i > 0) buf[j++] = temp[--i];
    buf[j] = '\0';
}

void thread_init(void) {
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_table[i].tid = 0;
        thread_table[i].state = THREAD_UNUSED;
        thread_table[i].next_blocked = 0;
    }
    // Main shell is TID 0
    thread_table[0].tid = 0;
    k_strncpy(thread_table[0].name, "shell_main", 31);
    thread_table[0].state = THREAD_RUNNING;
    current_thread = &thread_table[0];
}

int thread_create(const char *name, void (*entry_point)(void)) {
    int slot = -1;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].state == THREAD_UNUSED || 
            thread_table[i].state == THREAD_TERMINATED) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return -1;

    thread_t *t = &thread_table[slot];
    t->tid = next_tid++;
    k_strncpy(t->name, name, 31);
    t->name[31] = '\0';
    t->state = THREAD_READY;
    t->next_blocked = 0;

    uint32_t *stk = (uint32_t *)&t->stack[STACK_SIZE];
    stk = (uint32_t *)((uint32_t)stk & ~0x3);

    // Frame matching iret + irq0_stub restore
    *(--stk) = 0x202;                 // EFLAGS (IF set)
    *(--stk) = 0x08;                  // CS
    *(--stk) = (uint32_t)entry_point; // EIP
    *(--stk) = 0x202;                 // pushfd
    *(--stk) = 0;                     // EAX
    *(--stk) = 0;                     // ECX
    *(--stk) = 0;                     // EDX
    *(--stk) = 0;                     // EBX
    *(--stk) = 0;                     // ESI
    *(--stk) = 0;                     // EDI
    *(--stk) = 0;                     // EBP

    t->esp = (uint32_t)stk;
    return t->tid;
}

void thread_exit(void) {
    __asm__ volatile("cli");
    if (current_thread && current_thread->tid != 0) {
        current_thread->state = THREAD_TERMINATED;
    }
    __asm__ volatile("sti");
    schedule();
    while (1); // Never reached
}

void thread_block(void) {
    if (current_thread) {
        current_thread->state = THREAD_BLOCKED;
    }
    schedule();
}

void thread_unblock(thread_t *t) {
    if (t && t->state == THREAD_BLOCKED) {
        t->state = THREAD_READY;
    }
}

thread_t *thread_current(void) {
    return current_thread;
}

void thread_list(void) {
    vga_puts_color("\n  TID   STATE        NAME\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ───────────────────────────────\n");
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].state != THREAD_UNUSED) {
            char tid_str[12];
            int_to_str(thread_table[i].tid, tid_str);
            vga_puts("   ");
            vga_puts(tid_str);
            vga_puts("    ");

            switch (thread_table[i].state) {
                case THREAD_RUNNING:    vga_puts_color("RUNNING     ", VGA_LIGHT_GREEN, VGA_BLACK); break;
                case THREAD_READY:      vga_puts_color("READY       ", VGA_LIGHT_CYAN, VGA_BLACK); break;
                case THREAD_BLOCKED:    vga_puts_color("BLOCKED     ", VGA_LIGHT_MAGENTA, VGA_BLACK); break;
                case THREAD_TERMINATED: vga_puts_color("TERMINATED  ", VGA_DARK_GREY, VGA_BLACK); break;
                default:                vga_puts("UNKNOWN     "); break;
            }
            vga_puts(thread_table[i].name);
            vga_puts("\n");
        }
    }
    vga_puts("\n");
}
