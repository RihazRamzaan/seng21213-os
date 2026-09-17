#include "process.h"
#include "vga.h"

process_t process_table[MAX_PROCESSES];
process_t *current_process = 0;
static int next_pid = 1;

static void k_strcpy(char *dest, const char *src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

static void k_strncpy(char *dest, const char *src, size_t n) {
    while (n && *src) {
        *dest++ = *src++;
        n--;
    }
    while (n) {
        *dest++ = '\0';
        n--;
    }
}

static void int_to_str(int n, char *buf) {
    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    char temp[12];
    int i = 0, j = 0;
    while (n > 0) {
        temp[i++] = (n % 10) + '0';
        n /= 10;
    }
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
}

void process_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].pid = 0;
        process_table[i].state = PROCESS_UNUSED;
    }

    // PID 0 represents the main kernel/shell thread
    process_table[0].pid = 0;
    k_strcpy(process_table[0].name, "shell");
    process_table[0].state = PROCESS_RUNNING;
    current_process = &process_table[0];
}

int process_create(const char *name, void (*entry_point)(void)) {
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_UNUSED || 
            process_table[i].state == PROCESS_TERMINATED) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return -1;

    process_t *p = &process_table[slot];
    p->pid = next_pid++;
    k_strncpy(p->name, name, 31);
    p->name[31] = '\0';
    p->state = PROCESS_READY;

    // Top of stack
    uint32_t *stk = (uint32_t *)&p->stack[STACK_SIZE];
    stk = (uint32_t *)((uint32_t)stk & ~0x3);

    // Simulated interrupt frame for iret:
    *(--stk) = 0x202;                // EFLAGS (Interrupt flag set)
    *(--stk) = 0x08;                 // CS (Kernel Code Segment)
    *(--stk) = (uint32_t)entry_point;// EIP (Function to start)

    // General purpose registers + EFLAGS popped by irq0_stub:
    *(--stk) = 0x202;                // pushfd
    *(--stk) = 0;                    // EAX
    *(--stk) = 0;                    // ECX
    *(--stk) = 0;                    // EDX
    *(--stk) = 0;                    // EBX
    *(--stk) = 0;                    // ESI
    *(--stk) = 0;                    // EDI
    *(--stk) = 0;                    // EBP

    p->esp = (uint32_t)stk;
    return p->pid;
}

void process_kill(int pid) {
    if (pid <= 0) return; // Prevent killing root shell
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && process_table[i].state != PROCESS_UNUSED) {
            process_table[i].state = PROCESS_TERMINATED;
            return;
        }
    }
}

process_t *process_get_current(void) {
    return current_process;
}

void process_list(void) {
    vga_puts_color("\n  PID   STATE        NAME\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ───────────────────────────────\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state != PROCESS_UNUSED) {
            char pid_str[12];
            int_to_str(process_table[i].pid, pid_str);
            vga_puts("   ");
            vga_puts(pid_str);
            vga_puts("    ");

            switch (process_table[i].state) {
                case PROCESS_RUNNING:    vga_puts_color("RUNNING     ", VGA_LIGHT_GREEN, VGA_BLACK); break;
                case PROCESS_READY:      vga_puts_color("READY       ", VGA_LIGHT_CYAN, VGA_BLACK); break;
                case PROCESS_BLOCKED:    vga_puts_color("BLOCKED     ", VGA_LIGHT_MAGENTA, VGA_BLACK); break;
                case PROCESS_TERMINATED: vga_puts_color("TERMINATED  ", VGA_DARK_GREY, VGA_BLACK); break;
                default:                 vga_puts("UNKNOWN     "); break;
            }
            vga_puts(process_table[i].name);
            vga_puts("\n");
        }
    }
    vga_puts("\n");
}