#include "idt.h"
#include "scheduler.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

static struct idt_entry idt[256];
static struct idt_ptr idtp;

extern void irq0_stub(void);

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel       = sel;
    idt[num].always0   = 0;
    idt[num].flags     = flags;
}

void idt_init(void) {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base  = (uint32_t)&idt;

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // 0x08 is Kernel Code Segment; 0x8E = Present, 32-bit Interrupt Gate
    idt_set_gate(0x20, (uint32_t)irq0_stub, 0x08, 0x8E);

    __asm__ volatile("lidt (%0)" : : "r"(&idtp));
}

void pic_remap(void) {
    outb(PIC1_CMD, 0x11); io_wait();
    outb(PIC2_CMD, 0x11); io_wait();

    outb(PIC1_DATA, 0x20); io_wait(); // Master IRQ 0..7 -> 0x20..0x27
    outb(PIC2_DATA, 0x28); io_wait(); // Slave IRQ 8..15 -> 0x28..0x2F

    outb(PIC1_DATA, 0x04); io_wait();
    outb(PIC2_DATA, 0x02); io_wait();

    outb(PIC1_DATA, 0x01); io_wait();
    outb(PIC2_DATA, 0x01); io_wait();

    // Mask ALL hardware interrupts EXCEPT IRQ0 (Timer)
    // 0xFE = 11111110b -> Bit 0 is 0 (unmasked), Bit 1 (Keyboard) is 1 (masked)
    outb(PIC1_DATA, 0xFE);
    outb(PIC2_DATA, 0xFF);
}

void pit_init(uint32_t hz) {
    uint32_t divisor = 1193180 / hz;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

void timer_handler(void) {
    outb(0x20, 0x20); // Acknowledge EOI to PIC
    schedule();       // Updates current_process, switch.asm does the stack swap
}
