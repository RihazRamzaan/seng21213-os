/* =============================================================================
 * SENG21213-OS :: Main Kernel  (Stage 0 – Foundations)
 * File   : kernel/kernel.c
 *
 * PURPOSE
 *   This is the heart of your operating system. Right now it:
 *     1. Initialises VGA text-mode display
 *     2. Initialises the keyboard driver
 *     3. Prints a splash screen
 *     4. Runs a minimal interactive shell ("ksh")
 *
 * ASSIGNMENT MILESTONES  (what YOU will add in later lectures)
 *   Lecture  9  – Process Management  →  process.h / process.c / scheduler.c
 *   Lecture 10  – Threads             →  thread.h  / thread.c
 *   Lecture 11  – Memory Management   →  pmm.h     / pmm.c / vmm.c
 *   Lecture 12  – File System         →  fs.h      / fs.c
 *
 * CODING CONVENTION
 *   - Prefix kernel-internal functions with k_ (e.g. k_strcmp)
 *   - All driver APIs live in their own .h/.c pair
 *   - NEVER call malloc – use the PMM you build in Lecture 11
 * ============================================================================*/

#include "vga.h"
#include "keyboard.h"
#include "../include/types.h"
#include "process.h"
#include "scheduler.h"
#include "idt.h"
#include "thread.h"
#include "mutex.h"
#include "semaphore.h"
#include "pmm.h"
#include "fs.h"

/* ---------------------------------------------------------------------------
 * Forward declarations of shell commands
 * --------------------------------------------------------------------------*/
static int k_strcmp(const char *a, const char *b);
static int k_strncmp(const char *a, const char *b, size_t n);
static size_t k_strlen(const char *s);
static const char *k_ltrim(const char *s);
static void cmd_help(void);
static void cmd_clear(void);
static void cmd_about(void);
static void cmd_echo(const char *args);
static void cmd_mem(void);

/* ---------------------------------------------------------------------------
 * Utility: minimal string helpers (no libc in a freestanding kernel!)
 * --------------------------------------------------------------------------*/

static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

static int k_strncmp(const char *a, const char *b, size_t n) {
    while (n-- && *a && (*a == *b)) { a++; b++; }
    return n == (size_t)-1 ? 0 : (uint8_t)*a - (uint8_t)*b;
}

static size_t k_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

/* Skip leading spaces */
static const char *k_ltrim(const char *s) {
    while (*s == ' ') s++;
    return s;
}

/* ---------------------------------------------------------------------------
 * Stage 1: Demo Tasks & Commands
 * --------------------------------------------------------------------------*/
static void task_counter_a(void) {
    char ch = 'A';
    while (1) {
        vga_set_cursor(0, 78);
        vga_set_color(VGA_YELLOW, VGA_BLACK);
        vga_putchar(ch);
        ch = (ch == 'Z') ? 'A' : ch + 1;
        for (volatile int i = 0; i < 4000000; i++);
    }
}

static void task_counter_b(void) {
    char digit = '0';
    while (1) {
        vga_set_cursor(0, 79);
        vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        vga_putchar(digit);
        digit = (digit == '9') ? '0' : digit + 1;
        for (volatile int i = 0; i < 6000000; i++);
    }
}

static void cmd_ps(void) {
    process_list();
}

static void cmd_kill(const char *args) {
    args = k_ltrim(args);
    int pid = 0;
    while (*args >= '0' && *args <= '9') {
        pid = pid * 10 + (*args - '0');
        args++;
    }
    if (pid <= 0) {
        vga_puts_color("  Usage: kill <pid> (cannot kill PID 0)\n", VGA_YELLOW, VGA_BLACK);
        return;
    }
    process_kill(pid);
    vga_puts("  Process terminated.\n");
}

static void cmd_spawn(const char *args) {
    args = k_ltrim(args);
    if (k_strcmp(args, "counter_a") == 0) {
        process_create("counter_a", task_counter_a);
        vga_puts("  Spawned counter_a\n");
    } else if (k_strcmp(args, "counter_b") == 0) {
        process_create("counter_b", task_counter_b);
        vga_puts("  Spawned counter_b\n");
    } else {
        vga_puts_color("  Unknown task. Try: spawn counter_a or spawn counter_b\n", VGA_YELLOW, VGA_BLACK);
    }
}

/* ---------------------------------------------------------------------------
 * Add Concurrency Demo Tasks:
 * --------------------------------------------------------------------------*/

static mutex_t test_mutex;
static volatile int shared_counter = 0;

static void worker_thread_1(void) {
    for (int i = 0; i < 50000; i++) {
        mutex_lock(&test_mutex);
        shared_counter++;
        mutex_unlock(&test_mutex);
    }
    thread_exit();
}

static void worker_thread_2(void) {
    for (int i = 0; i < 50000; i++) {
        mutex_lock(&test_mutex);
        shared_counter++;
        mutex_unlock(&test_mutex);
    }
    thread_exit();
}

static void cmd_test_sync(void) {
    shared_counter = 0;
    mutex_init(&test_mutex);
    vga_puts("  Starting 2 threads incrementing shared_counter to 100,000 with mutex...\n");
    thread_create("worker_1", worker_thread_1);
    thread_create("worker_2", worker_thread_2);
}

/* ---------------------------------------------------------------------------
 * Add allocation test command:
 * --------------------------------------------------------------------------*/
static void cmd_mem_test(void) {
    vga_puts("  Allocating 3 physical pages...\n");
    void *p1 = pmm_alloc_page();
    void *p2 = pmm_alloc_page();
    void *p3 = pmm_alloc_page();

    vga_printf("  Page 1: 0x%x\n", (uint32_t)p1);
    vga_printf("  Page 2: 0x%x\n", (uint32_t)p2);
    vga_printf("  Page 3: 0x%x\n", (uint32_t)p3);

    vga_puts("  Freeing Page 2...\n");
    pmm_free_page(p2);

    void *p4 = pmm_alloc_page();
    vga_printf("  Allocated Page 4: 0x%x (should reuse Page 2)\n", (uint32_t)p4);

    pmm_free_page(p1);
    pmm_free_page(p3);
    pmm_free_page(p4);
    vga_puts("  Cleanup complete.\n");
}
/* ---------------------------------------------------------------------------
 *  RAM Disk File System:
 * --------------------------------------------------------------------------*/
static void cmd_touch(const char *name) {
    name = k_ltrim(name);
    if (k_strlen(name) == 0) {
        vga_puts_color("  Usage: touch <filename>\n", VGA_YELLOW, VGA_BLACK);
        return;
    }
    if (fs_create(name) == 0) {
        vga_puts("  Created file: ");
        vga_puts(name);
        vga_puts("\n");
    } else {
        vga_puts_color("  Error: file already exists or disk full.\n", VGA_LIGHT_RED, VGA_BLACK);
    }
}

static void cmd_cat(const char *name) {
    name = k_ltrim(name);
    if (k_strlen(name) == 0) {
        vga_puts_color("  Usage: cat <filename>\n", VGA_YELLOW, VGA_BLACK);
        return;
    }
    static char read_buf[1024];
    int res = fs_read(name, read_buf, sizeof(read_buf));
    if (res >= 0) {
        vga_puts("  ");
        vga_puts(read_buf);
        vga_puts("\n");
    } else {
        vga_puts_color("  Error: file not found.\n", VGA_LIGHT_RED, VGA_BLACK);
    }
}

static void cmd_write(const char *args) {
    args = k_ltrim(args);
    char filename[32];
    int i = 0;
    while (*args && *args != ' ' && i < 31) {
        filename[i++] = *args++;
    }
    filename[i] = '\0';
    args = k_ltrim(args);

    if (i == 0 || *args == '\0') {
        vga_puts_color("  Usage: write <filename> <text>\n", VGA_YELLOW, VGA_BLACK);
        return;
    }

    int res = fs_write(filename, args, k_strlen(args));
    if (res >= 0) {
        vga_puts("  Written to ");
        vga_puts(filename);
        vga_puts("\n");
    } else {
        vga_puts_color("  Error: file not found. Create it first with 'touch'.\n", VGA_LIGHT_RED, VGA_BLACK);
    }
}

/* ---------------------------------------------------------------------------
 * Splash Screen
 * --------------------------------------------------------------------------*/
static void print_splash(void) {
    vga_clear(VGA_BLACK);

    /* Top banner box */
    vga_draw_box(0, 0, 7, 80, VGA_LIGHT_MAGENTA);

    vga_set_cursor(1, 2);
    vga_puts_color("  SENG21213-OS  |  Computer Architecture & Operating Systems",
                   VGA_YELLOW, VGA_BLACK);

    vga_set_cursor(2, 2);
    vga_puts_color("  Stage 0: Kernel Foundations", VGA_LIGHT_CYAN, VGA_BLACK);

    vga_set_cursor(3, 2);
    vga_puts_color("  Faculty of Engineering – Department of Software Engineering",
                   VGA_LIGHT_GREY, VGA_BLACK);

    vga_set_cursor(4, 2);
    vga_puts_color("  Built by students, for students.  Type 'help' to begin.",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    vga_set_cursor(5, 2);
    vga_puts_color("  CPU: i686 (32-bit Protected Mode)  |  Display: VGA 80x25",
                   VGA_DARK_GREY, VGA_BLACK);

    vga_set_cursor(8, 0);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("  Welcome! This kernel was compiled from source and booted entirely\n");
    vga_puts("  from bare metal. There is no Linux or Windows underneath – only\n");
    vga_puts("  the code you and your team write.\n");
    vga_puts("\n");
    vga_puts("  Assignment milestones to implement:\n");
    vga_puts_color("    [L09] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Process Management  – PCB, ready queue, round-robin scheduler\n");
    vga_puts_color("    [L10] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Threads & Sync      – kernel threads, mutex, semaphore\n");
    vga_puts_color("    [L11] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Memory Management   – physical page allocator, virtual memory\n");
    vga_puts_color("    [L12] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("File System         – RAM disk, FAT-like directory structure\n");
    vga_puts("\n");
}

/* ---------------------------------------------------------------------------
 * Shell command implementations
 * --------------------------------------------------------------------------*/
static void cmd_help(void) {
    vga_puts_color("\n  SENG21213-OS Shell Commands\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  -------------------------------------------------------------\n");
    vga_puts("  help                - Show this command reference\n");
    vga_puts("  clear               - Clear screen\n");
    vga_puts("  about               - Kernel and system information\n");
    vga_puts("  echo <text>         - Print string to terminal\n");

    vga_puts_color("\n  Process & Task Management [L09]:\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ps                  - List all processes and runtime states\n");
    vga_puts("  spawn <task>        - Spawn background task (counter_a / counter_b)\n");
    vga_puts("  kill <pid>          - Terminate process by PID\n");

    vga_puts_color("\n  Threads & Synchronization [L10]:\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  threads             - List active kernel threads\n");
    vga_puts("  sync_test           - Test concurrent threads with mutex lock\n");

    vga_puts_color("\n  Memory Management [L11]:\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  free                - Display PMM physical page statistics & test\n");

    vga_puts_color("\n  RAMDisk & File System [L12]:\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ls                  - List files in RAMDisk directory\n");
    vga_puts("  touch <file>        - Create a new empty file\n");
    vga_puts("  write <file> <text> - Write data to a file\n");
    vga_puts("  cat <file>          - Display contents of a file\n\n");
}

static void cmd_clear(void) {
    vga_clear(VGA_BLACK);
}

static void cmd_about(void) {
    vga_puts_color("\n  About SENG21213-OS\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  Architecture : x86 (i686), 32-bit Protected Mode\n");
    vga_puts("  Bootloader   : Custom MBR (NASM)\n");
    vga_puts("  Kernel       : Freestanding C (GCC, no libc)\n");
    vga_puts("  VM Target    : QEMU (qemu-system-i386)\n");
    vga_puts("  Course       : SENG 21213 – Sem 2\n");
    vga_puts("  Reference    : Stallings, OS: Internals & Design Principles\n\n");
}

static void cmd_echo(const char *args) {
    vga_puts("  ");
    vga_puts(args);
    vga_puts("\n");
}

static void cmd_mem(void) {
    /* Stage 0 stub – students implement the real PMM in Lecture 11 */
    vga_puts_color("\n  Memory Map (stub – implement PMM in Lecture 11)\n",
                   VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  0x00000000 – 0x000FFFFF  :  First 1 MB (reserved/BIOS)\n");
    vga_puts("  0x00100000 – 0x00EFFFFF  :  Extended memory (usable ~14 MB)\n");
    vga_puts("  0x00F00000 – 0x00FFFFFF  :  BIOS / ROM area\n");
    vga_puts("  0xB8000    – 0xBFFFF     :  VGA frame buffer\n");
    vga_puts_color("\n  TODO: Use BIOS int 0x15, EAX=0xE820 to get real memory map\n\n",
                   VGA_YELLOW, VGA_BLACK);
}

/* ---------------------------------------------------------------------------
 * Shell process
 * --------------------------------------------------------------------------*/
static char  shell_buf[256];
static char  prompt[] = "\n  ksh> ";

static void shell_run(void) {
    vga_puts_color("\n  Kernel Shell ready. Type 'help' for commands.\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    while (true) {
        vga_puts_color(prompt, VGA_LIGHT_GREEN, VGA_BLACK);
        kb_readline(shell_buf, sizeof(shell_buf));

        /* Trim leading whitespace */
        const char *cmd = k_ltrim(shell_buf);
        if (k_strlen(cmd) == 0) continue;

        /* Dispatch */
        if (k_strcmp(cmd, "help")  == 0) { cmd_help();  continue; }
        if (k_strcmp(cmd, "clear") == 0) { cmd_clear(); continue; }
        if (k_strcmp(cmd, "about") == 0) { cmd_about(); continue; }
        if (k_strcmp(cmd, "mem")   == 0) { cmd_mem();   continue; }

        if (k_strncmp(cmd, "echo ", 5) == 0) {
            cmd_echo(k_ltrim(cmd + 5));
            continue;
        }

        /* Stage 1 Process Commands */
        if (k_strcmp(cmd, "ps") == 0) { cmd_ps(); continue; }
        if (k_strncmp(cmd, "kill", 4) == 0) { cmd_kill(cmd + 4); continue; }
        if (k_strncmp(cmd, "spawn", 5) == 0) { cmd_spawn(cmd + 5); continue; }
        if (k_strcmp(cmd, "threads") == 0) {
            thread_list();
            continue;
        }
        if (k_strcmp(cmd, "sync_test") == 0) {
            cmd_test_sync();
            continue;
        }
        if (k_strcmp(cmd, "free") == 0) {
            pmm_dump_info();
            cmd_mem_test();
            continue;
        }
        if (k_strcmp(cmd, "ls") == 0) {
        fs_list();
        continue;
        }
         if (k_strncmp(cmd, "cat", 3) == 0) {
        cmd_cat(cmd + 3);
        continue;
         }
         if (k_strncmp(cmd, "touch", 5) == 0) {
        cmd_touch(cmd + 5);
        continue;
          }
         if (k_strncmp(cmd, "write", 5) == 0) {
        cmd_write(cmd + 5);
        continue;
        }
        /* Milestone stubs */
        

        vga_puts_color("  Unknown command: ", VGA_LIGHT_RED, VGA_BLACK);
        vga_puts(cmd);
        vga_puts("\n  Type 'help' for a list of commands.\n");
    }
}

/* ---------------------------------------------------------------------------
 * Kernel entry point – called from kernel_entry.asm
 * --------------------------------------------------------------------------*/
void kernel_main(void) {
    /* 1. Core Hardware & Display */
    vga_init();
    kb_init();
    print_splash();

    /* 2. Memory & Storage Subsystems */
    pmm_init();
    fs_init();

    /* 3. Task Management */
    process_init();
    thread_init();
    scheduler_init();

    /* 4. Interrupts & Timer */
    idt_init();
    pic_remap();
    pit_init(50);

    /* 5. Launch interactive shell */
    /* Enable interrupts just before running the shell, or inside shell_run */
    __asm__ __volatile__("sti");

    shell_run();

    /* Catch unexpected exit */
    while (1) {
        __asm__ __volatile__("cli; hlt");
    }
}