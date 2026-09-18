#ifndef FS_H
#define FS_H

#include "../include/types.h"

#define MAX_FILES       32
#define MAX_FILENAME    32
#define MAX_FILE_SIZE   (4 * 512)

#define FS_TYPE_FILE    0
#define FS_TYPE_DIR     1

typedef struct {
    uint8_t  used;
    uint8_t  type;              // FS_TYPE_FILE or FS_TYPE_DIR
    char     name[MAX_FILENAME];
    uint32_t size;
    uint32_t direct_blocks[4];
    int      parent_inode;      // Inode of parent directory (-1 for root)
} inode_t;

void fs_init(void);
int  fs_create(const char *name);
int  fs_mkdir(const char *name);
int  fs_cd(const char *path);
void fs_pwd(char *buf);
int  fs_write(const char *name, const char *data, uint32_t len);
int  fs_read(const char *name, char *buf, uint32_t max_len);
void fs_list(void);

#endif