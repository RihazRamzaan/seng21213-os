#include "ramdisk.h"
#include "pmm.h"
#include "string.h"

// 1 MB RAMDisk memory backing
static uint8_t ramdisk_storage[RAMDISK_NUM_BLOCKS * RAMDISK_BLOCK_SIZE];

void ramdisk_init(void) {
    for (uint32_t i = 0; i < sizeof(ramdisk_storage); i++) {
        ramdisk_storage[i] = 0;
    }
}

int ramdisk_read_block(uint32_t block_num, uint8_t *buf) {
    if (block_num >= RAMDISK_NUM_BLOCKS || !buf) return -1;
    uint8_t *src = &ramdisk_storage[block_num * RAMDISK_BLOCK_SIZE];
    for (int i = 0; i < RAMDISK_BLOCK_SIZE; i++) {
        buf[i] = src[i];
    }
    return 0;
}

int ramdisk_write_block(uint32_t block_num, const uint8_t *buf) {
    if (block_num >= RAMDISK_NUM_BLOCKS || !buf) return -1;
    uint8_t *dest = &ramdisk_storage[block_num * RAMDISK_BLOCK_SIZE];
    for (int i = 0; i < RAMDISK_BLOCK_SIZE; i++) {
        dest[i] = buf[i];
    }
    return 0;
}
