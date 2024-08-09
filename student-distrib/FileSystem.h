#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include "types.h"

#define  BLOCK_SIZE  4096
#define MAX_NAME  32
uint32_t fsys_addr;     // starting point of the file system, initialized in kernel.c

typedef struct dentry_t {
    uint8_t  f_name[32];    // name of the file (up to 32B)
    uint32_t f_type;        // type of the file (0 for RTC, 1 for directory, 2 for regular file)
    uint32_t inode;         // inode index (should be ignored for RTC and directory type)
    uint8_t  reserved[24];  // 
} dentry_t;

typedef struct inode_t {
    uint32_t length;        // length of the file (in Byte)
    uint32_t idx_data[1023];// data block indexs
} inode_t;

typedef struct boot_block_t {
    uint32_t num_dentry;      // number of directory entries
    uint32_t num_inode;       // number of inode blocks (N)
    uint32_t num_data;        // number of data blocks (D)
    uint8_t  reserverd[52];
    dentry_t dentries[63];    // directory for up to 62 files
} boot_block_t;

typedef struct data_t {
    uint8_t data[4096];     // 4KB data block
} data_t;

void fsys_init();
int32_t read_dentry_by_name (const uint8_t* f_name, dentry_t* dentry);
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

// int32_t file_open_cp2(const uint8_t* filename);
// int32_t file_close_cp2(int32_t fd);
// int32_t file_read_cp2(int32_t idx_inode, uint32_t* offset_pointer, uint8_t* buf, int32_t nbytes);
// int32_t file_write_cp2(int32_t fd, const void* buf, int32_t nbytes);

// int32_t dir_open_cp2(const uint8_t* filename);
// int32_t dir_close_cp2(int32_t fd);
// int32_t dir_read_cp2(int32_t* idx_dir, void* buf, int32_t nbytes);
// int32_t dir_write_cp2(int32_t fd, const void* buf, int32_t nbytes);

int32_t file_open(const uint8_t* filename);
int32_t file_close(int32_t fd);
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);


int32_t dir_open(const uint8_t* directname);
int32_t dir_close(int32_t fd);
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes);

int32_t get_size(uint32_t inode);
uint32_t strlen_xjz(const uint8_t* string);
#endif /* FILESYSTEM_H */
