#ifndef RAMDISK_H
#define RAMDISK_H

#include "../include/types.h"

#define RAMDISK_BLOCK_SIZE  512
#define RAMDISK_NUM_BLOCKS  128  // 128 * 512 = 64 KB (fits easily in safe memory)

void ramdisk_init(void);
int  ramdisk_read_block(uint32_t block_num, uint8_t *buf);
int  ramdisk_write_block(uint32_t block_num, const uint8_t *buf);

#endif