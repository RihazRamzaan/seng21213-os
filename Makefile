# =============================================================================
# SENG21213-OS :: Makefile
# =============================================================================

# ---------------------------------------------------------------------------
# Toolchain
# ---------------------------------------------------------------------------
AS       := nasm
ASFLAGS  := -f elf32

# Try the cross-compiler first; fall back to native gcc -m32
ifneq (, $(shell which i686-elf-gcc 2>/dev/null))
    CC      := i686-elf-gcc
    LD      := i686-elf-ld
    CFLAGS  := -m32 -ffreestanding -fno-stack-protector -fno-pie -nostdlib \
               -Wall -Wextra -O2 -I./include
    LDFLAGS := -m elf_i386 -nostdlib
else
    CC      := gcc
    LD      := ld
    CFLAGS  := -m32 -ffreestanding -fno-stack-protector -fno-pie -nostdlib \
               -Wall -Wextra -O2 -I./include
    LDFLAGS := -m elf_i386 -nostdlib
endif

# ---------------------------------------------------------------------------
# Sources & Objects
# ---------------------------------------------------------------------------
BOOT_SRC  := boot/boot.asm
BOOT_BIN  := boot/boot.bin

# Assembly objects linked into the kernel
KERNEL_ENTRY_SRC := kernel/kernel_entry.asm
KERNEL_ENTRY_OBJ := build/kernel_entry.o

SWITCH_SRC       := boot/switch.asm
SWITCH_OBJ       := build/switch.o

# C Sources (Added process.c, scheduler.c, pit.c for Stage 1)
KERNEL_C_SRCS    := kernel/kernel.c    \
                    kernel/vga.c       \
                    kernel/keyboard.c  \
                    kernel/process.c   \
                    kernel/scheduler.c \
                    kernel/pit.c

KERNEL_C_OBJS    := $(patsubst kernel/%.c, build/%.o, $(KERNEL_C_SRCS))
KERNEL_ELF       := build/kernel.elf
KERNEL_BIN       := build/kernel.bin
OS_IMAGE         := seng21213-os.img

# ---------------------------------------------------------------------------
# Default target
# ---------------------------------------------------------------------------
.PHONY: all clean run run-debug info

all: $(OS_IMAGE)
	@echo ""
	@echo "  ✓  Build successful → $(OS_IMAGE)"
	@echo "  →  Run with:  make run"
	@echo ""

# ---------------------------------------------------------------------------
# Bootloader (Raw 512-byte binary)
# ---------------------------------------------------------------------------
$(BOOT_BIN): $(BOOT_SRC)
	@mkdir -p build
	@mkdir -p boot
	@echo "  [AS]  $<"
	$(AS) -f bin $< -o $@

# ---------------------------------------------------------------------------
# Kernel Assembly Objects
# ---------------------------------------------------------------------------
$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY_SRC)
	@mkdir -p build
	@echo "  [AS]  $<"
	$(AS) $(ASFLAGS) $< -o $@

$(SWITCH_OBJ): $(SWITCH_SRC)
	@mkdir -p build
	@echo "  [AS]  $<"
	$(AS) $(ASFLAGS) $< -o $@

# ---------------------------------------------------------------------------
# Kernel C Objects
# ---------------------------------------------------------------------------
build/%.o: kernel/%.c
	@mkdir -p build
	@echo "  [CC]  $<"
	$(CC) $(CFLAGS) -c $< -o $@

# ---------------------------------------------------------------------------
# Link kernel ELF, then extract flat binary
# ---------------------------------------------------------------------------
$(KERNEL_ELF): $(KERNEL_ENTRY_OBJ) $(SWITCH_OBJ) $(KERNEL_C_OBJS)
	@echo "  [LD]  $@"
	$(LD) $(LDFLAGS) -T linker.ld $^ -o $@

$(KERNEL_BIN): $(KERNEL_ELF)
	@echo "  [OBJCOPY] $@"
	objcopy -O binary $< $@

# ---------------------------------------------------------------------------
# Disk image: 1.44 MB floppy (boot sector + kernel)
# ---------------------------------------------------------------------------
$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	@echo "  [IMG]  Creating $(OS_IMAGE)..."
	dd if=/dev/zero  bs=512 count=2880 of=$(OS_IMAGE) 2>/dev/null
	dd if=$(BOOT_BIN)    conv=notrunc bs=512 count=1     of=$(OS_IMAGE) 2>/dev/null
	dd if=$(KERNEL_BIN)  conv=notrunc bs=512 seek=1      of=$(OS_IMAGE) 2>/dev/null
	@echo "  [IMG]  $(OS_IMAGE) ready ($(shell wc -c < $(KERNEL_BIN)) kernel bytes)"

# ---------------------------------------------------------------------------
# Run in QEMU
# ---------------------------------------------------------------------------
QEMU      := qemu-system-i386
QEMUFLAGS := -drive format=raw,file=$(OS_IMAGE) -m 32M

run: $(OS_IMAGE)
	@echo "  Starting QEMU... (Close window or press Ctrl+A X to exit)"
	$(QEMU) $(QEMUFLAGS)

# Debug: Pause at startup, expose GDB stub on port 1234
run-debug: $(OS_IMAGE)
	$(QEMU) $(QEMUFLAGS) -S -gdb tcp::1234 &
	@echo "  QEMU paused. Connect GDB: target remote :1234"

info:
	@echo "Toolchain: CC=$(CC)  AS=$(AS)  LD=$(LD)"

clean:
	rm -rf build $(BOOT_BIN) $(OS_IMAGE)
	@echo "  Cleaned."