#include "pmm.h"
#include "vga.h"

// Bitmap array: each byte holds 8 pages. 8192 pages / 8 = 1024 bytes
static uint8_t pmm_bitmap[TOTAL_PAGES / 8];
static uint32_t used_pages = 0;

// Linker script symbol marking where the kernel binary ends in memory
extern uint32_t _kernel_end;

static inline void bitmap_set(uint32_t page_idx) {
    pmm_bitmap[page_idx / 8] |= (1 << (page_idx % 8));
}

static inline void bitmap_clear(uint32_t page_idx) {
    pmm_bitmap[page_idx / 8] &= ~(1 << (page_idx % 8));
}

static inline int bitmap_test(uint32_t page_idx) {
    return (pmm_bitmap[page_idx / 8] & (1 << (page_idx % 8))) != 0;
}

void pmm_init(void) {
    // 1. Initially mark all memory as used/reserved
    for (uint32_t i = 0; i < sizeof(pmm_bitmap); i++) {
        pmm_bitmap[i] = 0xFF;
    }
    used_pages = TOTAL_PAGES;

    // 2. Determine end of kernel binary (round up to next page boundary)
    uint32_t kernel_end_addr = (uint32_t)&_kernel_end;
    if (kernel_end_addr < 0x100000) {
        // Kernel is loaded at 1 MB minimum
        kernel_end_addr = 0x200000;
    }
    uint32_t start_page = (kernel_end_addr + PAGE_SIZE - 1) / PAGE_SIZE;

    // 3. Mark usable free memory from end of kernel up to 32 MB
    for (uint32_t page = start_page; page < TOTAL_PAGES; page++) {
        bitmap_clear(page);
        used_pages--;
    }
}

void *pmm_alloc_page(void) {
    for (uint32_t i = 0; i < TOTAL_PAGES; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            used_pages++;
            return (void *)(i * PAGE_SIZE);
        }
    }
    return 0; // Out of memory
}

void pmm_free_page(void *ptr) {
    uint32_t addr = (uint32_t)ptr;
    if (addr % PAGE_SIZE != 0) return; // Must be page-aligned

    uint32_t page_idx = addr / PAGE_SIZE;
    if (page_idx >= TOTAL_PAGES) return;

    if (bitmap_test(page_idx)) {
        bitmap_clear(page_idx);
        if (used_pages > 0) used_pages--;
    }
}

uint32_t pmm_get_free_pages(void) {
    return TOTAL_PAGES - used_pages;
}

uint32_t pmm_get_used_pages(void) {
    return used_pages;
}

void pmm_dump_info(void) {
    uint32_t total_kb = (TOTAL_PAGES * PAGE_SIZE) / 1024;
    uint32_t used_kb  = (used_pages * PAGE_SIZE) / 1024;
    uint32_t free_kb  = ((TOTAL_PAGES - used_pages) * PAGE_SIZE) / 1024;

    vga_puts_color("\n  Physical Memory Status (PMM)\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ───────────────────────────────\n");
    vga_printf("  Total RAM  : %u KB (%u MB)\n", total_kb, total_kb / 1024);
    vga_printf("  Used Memory: %u KB (%u MB)\n", used_kb, used_kb / 1024);
    vga_printf("  Free Memory: %u KB (%u MB)\n", free_kb, free_kb / 1024);
    vga_printf("  Page Size  : %u bytes\n", PAGE_SIZE);
    vga_printf("  Total Pages: %u\n", TOTAL_PAGES);
    vga_printf("  Free Pages : %u\n\n", TOTAL_PAGES - used_pages);
}
