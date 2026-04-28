#include "fs.h"
#include <stdbool.h>

static super_block_t sb;
static int fat[DATA_LENGTH];
static dir_entry_t root_directory[MAX_FILES];
static bool is_mounted = false;
static fd_entry_t fdt[MAX_FDS];

static void init_memory(void)
{
    memset(&sb, 0, sizeof(sb));
    memset(fat, 0, sizeof(fat));
    memset(root_directory, 0, sizeof(root_directory));
    memset(fdt, 0, sizeof(fdt));
}

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

    init_memory();

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


    if (block_read(sb.dir_start, (char *)root_directory) != 0) {
        return -1;
    }

    for (int i = 0; i < MAX_FDS; ++i) {
        fdt[i].used = 0;
        fdt[i].dir_index = -1;
        fdt[i].offset = 0;
    }

    is_mounted = true;
    return 0;
}

int unmount_fs(char *disk_name){

    if (is_mounted == 0) {
        return -1; 
    }

    for (int i = 0; i < MAX_FDS; ++i) {
        fdt[i].used = 0;
        fdt[i].dir_index = -1;
        fdt[i].offset = 0;
    }

    if (block_write(0, (char *)&sb) != 0) {
        return -1;
    }
    

    for (int i = 0; i < sb.fat_length; i++) {
        if (block_write(sb.fat_start + i, (char *)fat + (i * 4096)) != 0) {
            return -1;
        }
    }

    if (block_write(sb.dir_start, (char *)root_directory) != 0) {
        return -1;
    }

    if (close_disk() != 0) {
        return -1;
    }

    is_mounted = false;
    return 0;
}

int fs_create(char *fname){
    if(!is_mounted){
        return -1;
    }

    size_t len = strlen(fname);
    if(len == 0 || len >= NAME_LENGTH){
        return -1;
    }

    int id_file = -1; //id of file
    for(int i = 0; i < MAX_FILES; i ++){
        if(root_directory[i].used == 1){
            //printf("used\n");
            if(strcmp(root_directory[i].name, fname) == 0){
                fprintf(stderr, "fs_open: file already made\n");
                return -1;
            }
        }
        else{
            //printf("id_file: %d\n", id_file);
            if(id_file == -1){
                id_file = i;
            }
        }
    }
    
    //printf("check id_file: %d\n", id_file);

    if(id_file == -1){
        fprintf(stderr, "fs_open: full, already have 64 files previously made\n");
        return -1;
    }

    strncpy(root_directory[id_file].name, fname, NAME_LENGTH - 1);
    root_directory[id_file].name[NAME_LENGTH - 1] = '\0'; 
    root_directory[id_file].used = 1;
    root_directory[id_file].size = 0;
    root_directory[id_file].head = -1;
    
    return 0;

}

int fs_open(char *fname){
    if(!is_mounted){
        return -1;
    }

    int dir_idx = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (root_directory[i].used == 1 && strcmp(root_directory[i].name, fname) == 0) {
            dir_idx = i;
            break;   
        }
    }

    if (dir_idx == -1) {
        return -1;
    }

    int fd_idx = -1;
    for (int i = 0; i < MAX_FDS; i++) {
        if (fdt[i].used == 0) {
            fd_idx = i;
            break;    
        }
    }

    if (fd_idx == -1) {
        return -1;
    }
    
    fdt[fd_idx].used = 1;
    fdt[fd_idx].dir_index = dir_idx;
    fdt[fd_idx].offset = 0;

    return fd_idx;

}

int fs_close(int fildes){
    if (!is_mounted) {
        fprintf(stderr, "fs_close: filesystem not mounted\n");
        return -1;
    }

    if (fildes < 0 || fildes >= MAX_FDS){
        return -1;
    }
    if (fdt[fildes].used == 0){
        return -1;
    } 

    fdt[fildes].used = 0;
    fdt[fildes].dir_index = -1;
    fdt[fildes].offset = 0;
    return 0;
}

int fs_delete(char *fname){
    if(!is_mounted){
        fprintf(stderr, "fs_delete: filesystem not mounted\n");
        return -1;
    }

    int dir_idx = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (root_directory[i].used == 1 && strcmp(root_directory[i].name, fname) == 0) {
            dir_idx = i;
            break;   
        }
    }

    if (dir_idx == -1) {
        fprintf(stderr, "fs_delete: file not found\n");
        return -1;
    }

    for(int i = 0; i < MAX_FDS; i ++){
        if(fdt[i].used == 1 && fdt[i].dir_index == dir_idx){
            fprintf(stderr, "fs_delete: file is open\n");
            return -1;
        }
    }

    int current_block = root_directory[dir_idx].head;

    while (current_block != -1) {
        int next_block = fat[current_block]; 
        fat[current_block] = -1; 
        current_block = next_block; 
    }

    root_directory[dir_idx].used = 0;
    root_directory[dir_idx].name[0] = '\0';
    root_directory[dir_idx].size = 0;
    root_directory[dir_idx].head = -1;

    return 0;
}

int fs_read(int fildes, void *buf, size_t nbyte){
    if(!is_mounted){
        fprintf(stderr, "fs_read: filesystem not mounted\n");
        return -1;
    }
    if(!buf){
        return -1;
    }
    if(fildes < 0 || fildes >= MAX_FDS){
        return -1;
    }
    if(!fdt[fildes].used){
        return -1;
    }

    int dir_idx = fdt[fildes].dir_index;
    int offset = fdt[fildes].offset;
    int file_size = root_directory[dir_idx].size;
    if(dir_idx < 0){
        return -1;
    }

    if(offset >= file_size){
        return 0; //EOF
    }

    if (offset + nbyte > file_size) {
        nbyte = file_size - offset; 
    }

    // 3. Fast-Forward to the correct starting block
    int current_block = root_directory[dir_idx].head;
    int blocks_to_skip = offset / 4096;
    
    for (int i = 0; i < blocks_to_skip; i++) {
        current_block = fat[current_block];
    }

    size_t bytes_read = 0;
    char bounce_buffer[4096];

    while (bytes_read < nbyte) {
        if (block_read(current_block + 4096, bounce_buffer) != 0) {
            return -1; 
        }

        int block_offset = (offset + bytes_read) % 4096;
        int bytes_left_to_write = nbyte - bytes_read;
        int space_in_block = 4096 - block_offset;
        int chunk_size;

        if (space_in_block < bytes_left_to_write) {
            chunk_size = space_in_block; // Fill the block to the brim
        } else {
            chunk_size = bytes_left_to_write; // Finish the user's request
        }

        memcpy((char*)buf + bytes_read, bounce_buffer + block_offset, chunk_size);

        bytes_read += chunk_size;

        if (bytes_read < nbyte) {
            current_block = fat[current_block];
        }
    }

    fdt[fildes].offset += bytes_read;

    return bytes_read;
}

int fs_write(int fildes, void *buf, size_t nbyte){
    if(!is_mounted){
        return -1;
    }
    if(fildes < 0 || fildes >= MAX_FDS){
        return -1;
    }
    if (fdt[fildes].used == 0){
        return -1;
    }

    int dir_idx = fdt[fildes].dir_index;
    int offset = fdt[fildes].offset;

    if (root_directory[dir_idx].head == -1) {
        int first_block = -1;
        
        // Search the FAT for a free block (Starting at 6)
        for (int i = 6; i < 4096; i++) { 
            if (fat[i] == -1) { 
                first_block = i;
                break;
            }
        }

        if (first_block == -1){
            return 0;
        }

        // Claim it for our file
        root_directory[dir_idx].head = first_block;
        fat[first_block] = -2; // Mark this new block as the EOF
    }

    int current_block = root_directory[dir_idx].head;
    int blocks_to_skip = offset / 4096;
    for (int i = 0; i < blocks_to_skip; i++) {
        current_block = fat[current_block];
    }

    size_t bytes_written = 0;
    char bounce_buffer[4096];
    
    while (bytes_written < nbyte) {
            if (block_read(current_block + 4096, bounce_buffer) != 0) {
                return -1; 
        }

        int block_offset = (offset + bytes_written) % 4096;
        int bytes_left_to_write = nbyte - bytes_written;
        int space_in_block = 4096 - block_offset;
        int chunk_size;

        if (space_in_block < bytes_left_to_write) {
            chunk_size = space_in_block; // Fill the block to the brim
        } else {
            chunk_size = bytes_left_to_write; // Finish the user's request
        }

        memcpy(bounce_buffer + block_offset, (char*)buf + bytes_written, chunk_size);

        if (block_write(current_block + 4096, bounce_buffer) != 0) {
            return -1; 
        }

        bytes_written += chunk_size;

        if (bytes_written < nbyte) {
            // If we hit -2, it means we are at the end of the file 
            // but we still have more data to write.
            if (fat[current_block] == -2) {
                int next_free = -1;
                // Search the FAT for a brand new empty block
                for (int i = 0; i < 4096; i++) {
                    if (fat[i] == -1) {
                        next_free = i;
                        break;
                    }
                }
                if (next_free == -1) break; // Out of space!

                // Link the chain: Old block points to New block
                fat[current_block] = next_free;
                // Set the new EOF marker
                fat[next_free] = -2;
            }
            
            current_block = fat[current_block];
        }

        fdt[fildes].offset += bytes_written;
        if (fdt[fildes].offset > root_directory[dir_idx].size) {
            root_directory[dir_idx].size = fdt[fildes].offset;
        }

        return bytes_written;
    }
}

int fs_get_filesize(int fildes) {
    if (!is_mounted || fildes < 0 || fildes >= MAX_FDS || fdt[fildes].used == 0) {
        return -1;
    }

    int dir_idx = fdt[fildes].dir_index;
    return root_directory[dir_idx].size;
}

int fs_lseek(int fildes, off_t offset) {
    if (!is_mounted || fildes < 0 || fildes >= MAX_FDS || fdt[fildes].used == 0) {
        return -1;
    }

    int dir_idx = fdt[fildes].dir_index;

    // Check: Is the new offset within the bounds of the file?
    if (offset < 0 || offset > root_directory[dir_idx].size) {
        return -1;
    }

    // Update the cursor in the File Descriptor Table
    fdt[fildes].offset = offset;

    return 0;
}

int fs_truncate(int fildes, off_t length) {
    if (is_mounted == 0 || fildes < 0 || fildes >= MAX_FDS || fdt[fildes].used == 0) {
        return -1;
    }

    int dir_idx = fdt[fildes].dir_index;

    // We can only truncate to a size smaller than or equal to the current size
    if (length < 0 || length > root_directory[dir_idx].size) {
        return -1;
    }

    if (length == 0) {
        int to_free = root_directory[dir_idx].head;
        while (to_free != -2 && to_free != -1) {
            int next_to_free = fat[to_free];
            fat[to_free] = -1;
            to_free = next_to_free;
        }
        root_directory[dir_idx].head = -1; // The file is now empty
        root_directory[dir_idx].size = 0;
        fdt[fildes].offset = 0;
        return 0;
    }

    int current_block = root_directory[dir_idx].head;
    int blocks_to_keep = length / 4096;
    
    // Walk the FAT to the block that will now be the EOF
    for (int i = 0; i < blocks_to_keep; i++) {
        current_block = fat[current_block];
    }

    int to_free = fat[current_block]; // This is the first block to be deleted
    fat[current_block] = -2;         // Set the new EOF marker

    // Loop through the rest of the old chain and mark them as -1 (FREE)
    while (to_free != -2) {
        int next_to_free = fat[to_free];
        fat[to_free] = -1;
        to_free = next_to_free;
    }

    //Update Metadata
    root_directory[dir_idx].size = length;

    // If the current offset is now "outside" the file, move it to the new end
    if (fdt[fildes].offset > length) {
        fdt[fildes].offset = length;
    }

    return 0;
}