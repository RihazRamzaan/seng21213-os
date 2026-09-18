#include "fs.h"
#include "ramdisk.h"
#include "vga.h"

static inode_t inode_table[MAX_FILES];
static uint32_t next_free_block = 10;
static int current_dir_inode = 0; // 0 is root "/"

static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

static void k_strncpy(char *dest, const char *src, size_t n) {
    while (n && *src) { *dest++ = *src++; n--; }
    while (n) { *dest++ = '\0'; n--; }
}

void fs_init(void) {
    ramdisk_init();
    for (int i = 0; i < MAX_FILES; i++) {
        inode_table[i].used = 0;
        inode_table[i].size = 0;
        inode_table[i].type = FS_TYPE_FILE;
        inode_table[i].parent_inode = -1;
        inode_table[i].name[0] = '\0';
        for (int b = 0; b < 4; b++) {
            inode_table[i].direct_blocks[b] = 0;
        }
    }
    next_free_block = 10;

    // Inode 0: Root directory "/"
    inode_table[0].used = 1;
    inode_table[0].type = FS_TYPE_DIR;
    k_strncpy(inode_table[0].name, "/", MAX_FILENAME - 1);
    inode_table[0].parent_inode = -1;
    current_dir_inode = 0;

    // Create default welcome file in root
    fs_create("welcome.txt");
    const char *msg = "Welcome to SENG21213-OS Hierarchical File System!";
    size_t len = 0;
    while (msg[len]) len++;
    fs_write("welcome.txt", msg, len);
}

int fs_create(const char *name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (inode_table[i].used && inode_table[i].parent_inode == current_dir_inode &&
            k_strcmp(inode_table[i].name, name) == 0) {
            return -1;
        }
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (!inode_table[i].used) {
            inode_table[i].used = 1;
            inode_table[i].type = FS_TYPE_FILE;
            inode_table[i].parent_inode = current_dir_inode;
            k_strncpy(inode_table[i].name, name, MAX_FILENAME - 1);
            inode_table[i].size = 0;
            for (int b = 0; b < 4; b++) {
                inode_table[i].direct_blocks[b] = next_free_block++;
            }
            return 0;
        }
    }
    return -1;
}

int fs_mkdir(const char *name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (inode_table[i].used && inode_table[i].parent_inode == current_dir_inode &&
            k_strcmp(inode_table[i].name, name) == 0) {
            return -1;
        }
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (!inode_table[i].used) {
            inode_table[i].used = 1;
            inode_table[i].type = FS_TYPE_DIR;
            inode_table[i].parent_inode = current_dir_inode;
            k_strncpy(inode_table[i].name, name, MAX_FILENAME - 1);
            inode_table[i].size = 0;
            return 0;
        }
    }
    return -1;
}

int fs_cd(const char *path) {
    if (k_strcmp(path, "/") == 0) {
        current_dir_inode = 0;
        return 0;
    }
    if (k_strcmp(path, ".") == 0) {
        return 0;
    }
    if (k_strcmp(path, "..") == 0) {
        if (inode_table[current_dir_inode].parent_inode != -1) {
            current_dir_inode = inode_table[current_dir_inode].parent_inode;
        }
        return 0;
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (inode_table[i].used &&
            inode_table[i].parent_inode == current_dir_inode &&
            inode_table[i].type == FS_TYPE_DIR &&
            k_strcmp(inode_table[i].name, path) == 0) {
            current_dir_inode = i;
            return 0;
        }
    }
    return -1;
}

void fs_pwd(char *buf) {
    if (current_dir_inode == 0) {
        buf[0] = '/';
        buf[1] = '\0';
        return;
    }

    int curr = current_dir_inode;
    char names[8][MAX_FILENAME];
    int count = 0;

    while (curr > 0 && count < 8) {
        k_strncpy(names[count++], inode_table[curr].name, MAX_FILENAME - 1);
        curr = inode_table[curr].parent_inode;
    }

    int pos = 0;
    for (int i = count - 1; i >= 0; i--) {
        buf[pos++] = '/';
        const char *s = names[i];
        while (*s) buf[pos++] = *s++;
    }
    buf[pos] = '\0';
}

int fs_write(const char *name, const char *data, uint32_t len) {
    if (len > MAX_FILE_SIZE) len = MAX_FILE_SIZE;

    for (int i = 0; i < MAX_FILES; i++) {
        if (inode_table[i].used && inode_table[i].parent_inode == current_dir_inode &&
            inode_table[i].type == FS_TYPE_FILE && k_strcmp(inode_table[i].name, name) == 0) {
            inode_table[i].size = len;
            uint32_t written = 0;
            uint8_t block_buf[RAMDISK_BLOCK_SIZE];

            for (int b = 0; b < 4 && written < len; b++) {
                uint32_t to_write = len - written;
                if (to_write > RAMDISK_BLOCK_SIZE) to_write = RAMDISK_BLOCK_SIZE;

                for (uint32_t j = 0; j < to_write; j++) {
                    block_buf[j] = (uint8_t)data[written + j];
                }
                ramdisk_write_block(inode_table[i].direct_blocks[b], block_buf);
                written += to_write;
            }
            return written;
        }
    }
    return -1;
}

int fs_read(const char *name, char *buf, uint32_t max_len) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (inode_table[i].used && inode_table[i].parent_inode == current_dir_inode &&
            inode_table[i].type == FS_TYPE_FILE && k_strcmp(inode_table[i].name, name) == 0) {
            uint32_t to_read = inode_table[i].size;
            if (to_read > max_len - 1) to_read = max_len - 1;

            uint32_t bytes_read = 0;
            uint8_t block_buf[RAMDISK_BLOCK_SIZE];

            for (int b = 0; b < 4 && bytes_read < to_read; b++) {
                ramdisk_read_block(inode_table[i].direct_blocks[b], block_buf);
                uint32_t chunk = to_read - bytes_read;
                if (chunk > RAMDISK_BLOCK_SIZE) chunk = RAMDISK_BLOCK_SIZE;

                for (uint32_t j = 0; j < chunk; j++) {
                    buf[bytes_read + j] = (char)block_buf[j];
                }
                bytes_read += chunk;
            }
            buf[bytes_read] = '\0';
            return bytes_read;
        }
    }
    return -1;
}

void fs_list(void) {
    vga_puts_color("\n  TYPE   SIZE (B)   NAME\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ----   --------   ----------------\n");

    int count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (inode_table[i].used && i != 0 && inode_table[i].parent_inode == current_dir_inode) {
            if (inode_table[i].type == FS_TYPE_DIR) {
                vga_puts_color("  DIR    ", VGA_LIGHT_CYAN, VGA_BLACK);
                vga_puts("     -      ");
            } else {
                vga_puts("  FILE   ");
                vga_printf("%u", inode_table[i].size);
                if (inode_table[i].size < 10)          vga_puts("          ");
                else if (inode_table[i].size < 100)    vga_puts("         ");
                else if (inode_table[i].size < 1000)   vga_puts("        ");
                else                                  vga_puts("       ");
            }

            vga_puts(inode_table[i].name);
            vga_puts("\n");
            count++;
        }
    }

    if (count == 0) {
        vga_puts("  (empty directory)\n");
    }
    vga_puts("\n");
}