#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fs.h"

char test_data[4096]; 
#define DISK_NAME "vdisk"

void* reader_thread(void* arg) {
    mount_fs(DISK_NAME);
    
    int fd = fs_open("fileA");
    char buffer[4096];
    fs_read(fd, buffer, 4096);
    fs_close(fd);
    
    unmount_fs(DISK_NAME);
    return NULL;
}

void* copier_tester_thread(void* arg) {
    mount_fs(DISK_NAME);
    
    // Copy fileA to fileB
    fs_create("fileB");
    int fd_old = fs_open("fileA");
    int fd_new = fs_open("fileB");
    
    char copy_buf[4096];
    fs_read(fd_old, copy_buf, 4096);
    fs_write(fd_new, copy_buf, 4096);
    
    fs_close(fd_old);
    fs_close(fd_new);
    
    // Delete original file
    fs_delete("fileA");
    
    // Test remaining functions
    int fd_test = fs_open("fileB");
    fs_get_filesize(fd_test);
    fs_truncate(fd_test, 100);
    fs_close(fd_test);
    
    unmount_fs(DISK_NAME);
    return NULL;
}

int main() {
    memset(test_data, 'X', 4096);

    // Create, write, close, and unmount
    make_fs(DISK_NAME);
    mount_fs(DISK_NAME);
    
    fs_create("fileA");
    int fd = fs_open("fileA");
    fs_write(fd, test_data, 4096);
    fs_close(fd);
    
    unmount_fs(DISK_NAME);

    // Launch threads sequentially
    pthread_t t1, t2;
    pthread_create(&t1, NULL, reader_thread, NULL);
    pthread_join(t1, NULL); 
    
    pthread_create(&t2, NULL, copier_tester_thread, NULL);
    pthread_join(t2, NULL);

    return 0;
}