#include "fs.h"
#include <stdbool.h>

static super_block_t sb;
static int fat[DATA_LENGTH];
static dir_entry_t root_directory[MAX_FILES];
static bool is_mounted = false;

int make_fs(char *disk_name){

    //int err = make_disk(disk_name);

    if(make_disk(disk_name) != 0){
        //printf("%d\n", err);
        fprintf(stderr, "filesystem_make_fs: make_disk failed\n");
        return -1;
    }

    //err = open_disk(disk_name);

    if(open_disk(disk_name) != 0){
        fprintf(stderr, "filesystem_make_fs: open_disk failed\n");
        return -1;
    }

    sb.magic = FILE_MAGIC_NUMBER;
    sb.block_size = BLOCK_SIZE;

    sb.fat_start = FAT_START;
    sb.fat_length = FAT_LENGTH; //FAT BLOCK COUNT

    sb.dir_start = DIR_START;
    sb.dir_length = DIR_LENGTH;

    sb.data_start = DATA_START;
    sb.data_length = DATA_LENGTH; //DATA BLOCK COUNT

    sb.total_blocks = DISK_BLOCKS;

    //int fat[DATA_LENGTH];
    for(int i = 0; i < DATA_LENGTH; i ++){
        fat[i] = -1;
    }

    //dir_entry_t root_directory[MAX_FILES];
    for(int i = 0; i < MAX_FILES; i ++){
        root_directory[i].used = 0;
        root_directory[i].head = -1;
        root_directory[i].size = 0;
        root_directory[i].name[0]= '\0';
    }

    if(block_write(0, (char *)&sb) != 0){
        return -1;
    }

    for(int i = 0; i < FAT_LENGTH; i ++){
        if(block_write(sb.fat_start + i, (char *)fat + (i*4096)) != 0){
            return -1;
        }
    }

    if(block_write(sb.dir_start, (char *)root_directory) != 0){
        return -1;
    }

    if(close_disk()!=0){
        return -1;
    }

    return 0;
}

int mount_fs(char *disk_name){
    
    if(is_mounted){
        fprintf(stderr, "fs_mount_fs: mounted already\n");
        return -1;
    }

    if(open_disk(disk_name) != 0){
        fprintf(stderr, "fs_mount_fs: failed to open disk\n");
        return -1;
    }

    if (block_read(0, (char *)&sb) != 0) {
        return -1;
    }

    if(sb.magic != FILE_MAGIC_NUMBER){
        close_disk();
        return -1;
    }

    for (int i = 0; i < sb.fat_length; i++) {
        if (block_read(sb.fat_start + i, (char *)fat + (i*4096)) != 0) {
            return -1;
        }
    }


    if (block_read(sb.dir_start, (char *)directory) != 0) {
        return -1;
    }

    is_mounted = true;
    return 0;
}

int unmount_fs(char *disk_name){

    if (is_mounted == 0) {
        return -1; 
    }

    if (block_write(0, (char *)&sb) != 0) {
        return -1;
    }

    for (int i = 0; i < sb.fat_length; i++) {
        if (block_write(sb.fat_start + i, (char *)fat + (i * 4096)) != 0) {
            return -1;
        }
    }

    if (block_write(sb.dir_start, (char *)directory) != 0) {
        return -1;
    }

    if (close_disk() != 0) {
        return -1;
    }

    is_mounted = false;
    return 0;
}