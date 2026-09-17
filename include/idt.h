#ifndef IDT_H
#define IDT_H

#include "../include/types.h"

struct idt_entry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

void idt_init(void);
void pic_remap(void);
void pit_init(uint32_t hz);

#endif
