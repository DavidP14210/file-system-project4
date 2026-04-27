#ifndef FS_H
#define FS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "disk.h"

#define FILE_MAGIC_NUMBER 0x12345678
#define FAT_START 1
#define FAT_LENGTH 4
#define DIR_START 5
#define DIR_LENGTH 1
#define DATA_START 4096
#define DATA_LENGTH 4096

#define NAME_LENGTH 16
#define MAX_FILES 64
#define MAX_FDS 32


int make_fs(char *disk_name);
int mount_fs(char *disk_name);
int umount_fs(char *disk_name);

int fs_open(char *fname);
int fs_close(int fildes);
int fs_create(char *fname);
int fs_delete(char *fname);
int fs_read(int fildes, void *buf, size_t nbyte);
int fs_write(int fildes, void *buf, size_t nbye);
int fs_get_filesize(int fildes);
int fs_lseek(int fildes, off_t offset);
int fs_truncate(int fildes, off_t length);

typedef struct {
    int magic;            // identifier (e.g., 0x12345678)
    int block_size;       // size of each block (e.g., 4096 bytes)
    
    int fat_start;        // starting block of FAT
    int fat_length;       // number of blocks FAT occupies

    int dir_start;        // starting block of root directory
    int dir_length;       // number of blocks for directory

    int data_start;       // starting block of data region
    int data_length;      // should be 4096 (REQUIRED)

    int total_blocks;     // total blocks in disk
} super_block_t;

typedef struct {
    char name[NAME_LENGTH];
    int size;
    int head;
    int used;
    int padding;
} dir_entry_t;

typedef struct {
    int used;       // 0 if empty, 1 if currently open
    int dir_index;  // The index (0-63) of this file in the directory array
    int offset;     // The current byte position for reading/writing
} fd_entry_t;

#endif