#include "fs.h"
#include "ramdisk.h"
#include "vga.h"

static inode_t inode_table[MAX_FILES];
static uint32_t next_free_block = 10; // Reserve first 10 blocks for metadata

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
        inode_table[i].name[0] = '\0';
        for (int b = 0; b < 4; b++) {
            inode_table[i].direct_blocks[b] = 0;
        }
    }
    next_free_block = 10;

    // Create a default welcome file
    fs_create("welcome.txt");
    const char *msg = "Welcome to SENG21213-OS RAMDisk File System!";
    size_t len = 0;
    while (msg[len]) len++;
    fs_write("welcome.txt", msg, len);
}

int fs_create(const char *name) {
    // Check if file already exists
    for (int i = 0; i < MAX_FILES; i++) {
        if (inode_table[i].used && k_strcmp(inode_table[i].name, name) == 0) {
            return -1;
        }
    }

    // Find free inode
    for (int i = 0; i < MAX_FILES; i++) {
        if (!inode_table[i].used) {
            inode_table[i].used = 1;
            k_strncpy(inode_table[i].name, name, MAX_FILENAME - 1);
            inode_table[i].size = 0;
            for (int b = 0; b < 4; b++) {
                inode_table[i].direct_blocks[b] = next_free_block++;
            }
            return 0;
        }
    }
    return -1; // File table full
}

int fs_write(const char *name, const char *data, uint32_t len) {
    if (len > MAX_FILE_SIZE) len = MAX_FILE_SIZE;

    for (int i = 0; i < MAX_FILES; i++) {
        if (inode_table[i].used && k_strcmp(inode_table[i].name, name) == 0) {
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
    return -1; // File not found
}

int fs_read(const char *name, char *buf, uint32_t max_len) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (inode_table[i].used && k_strcmp(inode_table[i].name, name) == 0) {
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
    return -1; // File not found
}

void fs_list(void) {
    vga_puts_color("\n  SIZE (B)   FILENAME\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  --------   ----------------\n");
    
    int count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (inode_table[i].used) {
            vga_puts("  ");
            vga_printf("%u", inode_table[i].size);
            
            // Manual padding so columns align cleanly without %-8u
            if (inode_table[i].size < 10)          vga_puts("          ");
            else if (inode_table[i].size < 100)    vga_puts("         ");
            else if (inode_table[i].size < 1000)   vga_puts("        ");
            else                                  vga_puts("       ");
            
            vga_puts(inode_table[i].name);
            vga_puts("\n");
            count++;
        }
    }
    
    if (count == 0) {
        vga_puts("  (no files)\n");
    }
    vga_puts("\n");
}