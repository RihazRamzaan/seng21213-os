#ifndef FS_H
#define FS_H

#include "../include/types.h"

#define MAX_FILES       32
#define MAX_FILENAME    32
#define MAX_FILE_SIZE   (4 * 512) // 2 KB max file size (4 blocks)

typedef struct {
    uint8_t  used;
    char     name[MAX_FILENAME];
    uint32_t size;
    uint32_t direct_blocks[4];
} inode_t;

void fs_init(void);
int  fs_create(const char *name);
int  fs_write(const char *name, const char *data, uint32_t len);
int  fs_read(const char *name, char *buf, uint32_t max_len);
void fs_list(void);

#endif
