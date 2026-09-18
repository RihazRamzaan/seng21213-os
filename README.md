# SENG21213-OS — Educational x86 Operating System

> **Course**: SENG 21213 – Computer Architecture & Operating Systems  
> **Year**: 2nd Year, Software Engineering  
> **Target Architecture**: x86 (32-bit Protected Mode, Bare Metal)  
> **Current Version**: `v0.5-stage4` (Stages 0–4 Complete + Hierarchical Subdirectory Extension)

---

## Overview

**SENG21213-OS** is a monolithic educational operating system written from scratch in C and x86 Assembly (NASM). Developed across five lecture milestones (Lectures 08–12), this kernel transitions from a 16-bit real-mode MBR bootloader into a preemptive multitasking operating system featuring:
* Interactive command shell (`ksh`)
* Hardware timer interrupts and process control blocks (PCBs)
* Assembly-level context switching
* Kernel threads, binary mutexes, and counting semaphores
* Bitmap-based physical page allocator (PMM)
* In-memory block device (RAMDisk)
* Inode-based hierarchical file system supporting subdirectories

---

## Architectural Milestones & Subsystems

| Milestone | Stage & Architectural Scope | Key Source Files | Status |
|---|---|---|---|
| **L08** | **Stage 0: Kernel Foundations**<br>MBR bootloader (16-bit real mode), Global Descriptor Table (GDT), transition to 32-bit protected mode, VGA 80×25 text driver, PS/2 keyboard polling driver, minimal shell (`ksh`). | `boot/boot.asm`<br>`kernel/kernel_entry.asm`<br>`kernel/vga.c`<br>`kernel/keyboard.c` | Completed |
| **L09** | **Stage 1: Preemptive Multitasking**<br>Dual 8259 PIC remapping, 8253/8254 PIT (50 Hz), Interrupt Descriptor Table (IDT), PCB lifecycle management, assembly context switching (`switch.asm`). | `kernel/process.c`<br>`kernel/scheduler.c`<br>`kernel/pit.c`<br>`boot/switch.asm` | Completed |
| **L10** | **Stage 2: Kernel Threads & Synchronization**<br>Thread Control Blocks (TCBs), non-spinning binary mutexes with blocked wait-queues, counting semaphores. | `kernel/thread.c`<br>`kernel/mutex.c`<br>`kernel/semaphore.c` | Completed |
| **L11** | **Stage 3: Physical Memory Management (PMM)**<br>Bitmap page-frame allocator managing 32 MB of physical RAM (8,192 4 KB pages), low memory and kernel image reservation. | `kernel/pmm.c`<br>`include/pmm.h`<br>`linker.ld` | Completed |
| **L12** | **Stage 4: File System & RAMDisk**<br>512-byte sector RAMDisk block device, inode table, file creation, direct-block indexing, multi-block read/write operations. | `kernel/ramdisk.c`<br>`kernel/fs.c`<br>`include/fs.h` | Completed |

---

## Implemented Custom Extensions

Beyond the baseline requirements, the following architectural extensions and tools were implemented:

### 1. Hierarchical Directory Support (`mkdir`, `cd`, `pwd`)
* **Inode Typing**: Inodes differentiate between regular files (`FS_TYPE_FILE`) and directory containers (`FS_TYPE_DIR`).
* **Tree Tracking**: Inodes store a `parent_inode` reference, establishing parent-child directory relationships.
* **Path Resolution**: Supports absolute jumps to root (`/`), relative current directory (`.`), relative parent directory (`..`), and child folder navigation.
* **Working Directory Tracking**: All file operations (`ls`, `touch`, `write`, `cat`) dynamically scope to the current working directory (`current_dir_inode`).
* **Canonical Path Printing**: `pwd` traces parent links back to root to resolve the full directory path string.

### 2. Concurrency & Preemption Demonstrators
* **VGA Corner Counters (`spawn`)**: Dual background tasks (`counter_a`, `counter_b`) write continuous numeric counters directly to VGA display memory offsets concurrently while the shell continues accepting input.
* **Lock-Contention Stress Test (`sync_test`)**: Two concurrent worker threads increment a shared counter under mutex protection across preemptive timer ticks to demonstrate race-free execution.

### 3. CP437-Compliant VGA Formatting
* Formatted table borders and file listings using standard ASCII characters to prevent IBM CP437 encoding artifacts on bare-metal VGA consoles.
* Manual digit-length padding logic to replace unimplemented `printf` width specifiers (`%-8u`).

---

## Interactive Shell Command Reference (`ksh`)

Type `help` inside `ksh` to display the full command manual:

```text
  SENG21213-OS Shell Commands
  -------------------------------------------------------------
  help                - Show this command reference
  clear               - Clear screen
  about               - Kernel and system information
  echo <text>         - Print string to terminal

  Process & Task Management [L09]:
  ps                  - List all processes and runtime states
  spawn <task>        - Spawn background task (counter_a / counter_b)
  kill <pid>          - Terminate process by PID

  Threads & Synchronization [L10]:
  threads             - List active kernel threads
  sync_test           - Test concurrent threads with mutex lock

  Memory Management [L11]:
  free                - Display PMM physical page statistics & test

  RAMDisk & File System [L12 + Extensions]:
  ls                  - List files and folders in current directory
  pwd                 - Print current working directory path
  cd <path>           - Change directory (supports '/', '..', name)
  mkdir <dir>         - Create a new subdirectory
  touch <file>        - Create a new empty file
  write <file> <text> - Write string data into a file
  cat <file>          - Display contents of a file
```

---

## Directory Structure

```
seng21213-os/
├── boot/
│   ├── boot.asm             # MBR Bootloader (16-bit real mode -> 32-bit protected mode)
│   └── switch.asm           # Assembly context switcher and IRQ0 ISR handler
├── kernel/
│   ├── kernel_entry.asm     # Protected mode entry point calling kernel_main()
│   ├── kernel.c             # Kernel initialization, shell loop, command dispatch
│   ├── vga.c                # VGA 80x25 text-mode console driver & vga_printf
│   ├── keyboard.c           # PS/2 keyboard driver & scancode translation table
│   ├── process.c            # Process Control Block (PCB) management & stack setup
│   ├── scheduler.c          # Round-robin preemptive task scheduler
│   ├── pit.c                # 8259 PIC remapping & 8254 PIT timer driver (50 Hz)
│   ├── thread.c             # Thread interfaces, blocking & unblocking
│   ├── mutex.c              # Mutex lock/unlock with linked-list wait queues
│   ├── semaphore.c          # Counting semaphores (sem_wait, sem_post)
│   ├── pmm.c                # Physical Memory Manager (page bitmap allocator)
│   ├── ramdisk.c            # In-memory block device driver (512-byte sectors)
│   └── fs.c                 # Inode file system with hierarchical subdirectories
├── include/
│   ├── types.h              # Primitive integer types (uint8_t, uint32_t, size_t)
│   ├── vga.h                # VGA text mode prototypes and color attributes
│   ├── keyboard.h           # PS/2 keyboard driver prototypes
│   ├── process.h            # PCB definitions and process lifecycle states
│   ├── scheduler.h          # Scheduler declarations
│   ├── idt.h                # IDT descriptor layout and PIC definitions
│   ├── thread.h             # Thread structures and function prototypes
│   ├── mutex.h              # Mutex structures and synchronization prototypes
│   ├── semaphore.h          # Semaphore structures and synchronization prototypes
│   ├── pmm.h                # Page allocation constants (PAGE_SIZE, TOTAL_PAGES)
│   ├── ramdisk.h            # RAMDisk sector sizing and block I/O declarations
│   └── fs.h                 # Inode structures, directory constants, and FS API
├── linker.ld                # Linker script mapping kernel code at 0x10000
├── Makefile                 # Automated build, link, and emulation targets
└── README.md                # System documentation
```

---

## Build & Execution Instructions

### Prerequisites

* **NASM** (Netwide Assembler)
* **GCC** (with 32-bit multilib: `gcc-multilib`)
* **GNU Binutils** (`ld`, `objcopy`)
* **QEMU** (`qemu-system-i386`)
* **GNU Make**

On Ubuntu / Debian / WSL2:

```bash
sudo apt update
sudo apt install -y nasm gcc gcc-multilib binutils qemu-system-x86 make
```

### Build the Image

To assemble, compile, and link the raw bootable floppy image (`seng21213-os.img`):

```bash
make clean
make
```

### Run in QEMU

Launch the operating system in QEMU configured with 32 MB of RAM:

```bash
make run
```

### Debugging with GDB

To start QEMU with a GDB stub listening on TCP port 1234:

```bash
make run-debug
```

In a separate terminal window:

```bash
gdb -ex "target remote :1234" -ex "symbol-file build/kernel.elf"
```

---

## System Verification Workflows

### 1. Process Preemption

```text
ksh> spawn counter_a
ksh> spawn counter_b
ksh> ps
```

* Observe numbers incrementing in real time at the VGA display edges while the shell accepts new commands without latency.

### 2. Mutex Synchronization

```text
ksh> sync_test
ksh> threads
```

* Spawns worker threads that increment a shared resource under mutex lock protection without data corruption.

### 3. Physical Page Allocator

```text
ksh> free
```

* Displays total, used, and free memory pools, exercises physical page allocation, frees an intermediate page, and confirms immediate address slot reuse.

### 4. Hierarchical File System & Directories

```text
ksh> ls
ksh> touch doc.txt
ksh> write doc.txt Operating Systems Assignment
ksh> cat doc.txt
ksh> mkdir mydir
ksh> cd mydir
ksh> pwd
ksh> touch inner.txt
ksh> write inner.txt nested file contents
ksh> ls
ksh> cd ..
ksh> pwd
ksh> ls
```

---

## Git Release Tag History

* **`v0.1-stage0`**: Real-mode bootloader, GDT setup, 32-bit protected mode, VGA text console, polling keyboard driver, and minimal shell.
* **`v0.2-stage1`**: Preemptive round-robin scheduler, 8254 PIT (50 Hz), 8259 PIC remapping, PCB lifecycle tracking, and assembly context switching (`switch.asm`).
* **`v0.3-stage2`**: Kernel threads, non-spinning binary mutexes with blocked wait-queues, and counting semaphores.
* **`v0.4-stage3`**: Physical Memory Management with bitmap page-frame allocation managing 32 MB of RAM and low-memory reservation.
* **`v0.5-stage4`**: RAMDisk block device, inode file system, direct block index tables, and hierarchical directory extension (`mkdir`, `cd`, `pwd`).
