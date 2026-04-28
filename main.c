#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "disk.h"
#include "fs.h"

#define DISK_NAME "vdisk"

int main() {
    if (make_fs(DISK_NAME) != 0) {
        printf("Failed to make_fs\n");
        return -1;
    }

    if (mount_fs(DISK_NAME) != 0) {
        printf("Failed to mount_fs\n");
        return -1;
    }

    printf("Disk initialized and mounted successfully!\n");

    fs_create("hello.txt");

    // 4. Open the file to get a File Descriptor (fildes)
    int fd = fs_open("hello.txt");
    assert(fd >= 0);

    // 5. Write data
    char *input = "Hello World!";
    int bytes_written = fs_write(fd, input, strlen(input));
    printf("Wrote %d bytes to hello.txt\n", bytes_written);

    // 6. Seek back to the beginning so we can read
    fs_lseek(fd, 0);

    // 7. Read the data back
    char output[20];
    memset(output, 0, 20); // Clear the buffer
    int bytes_read = fs_read(fd, output, strlen(input));
    
    printf("Read back: '%s' (%d bytes)\n", output, bytes_read);

    // 8. Verify the data matches
    if (strcmp(input, output) == 0) {
        printf("SUCCESS: Data matches!\n");
    } else {
        printf("FAILURE: Data mismatch!\n");
    }

    fs_close(fd);

    // --- We will add the next steps here ---

    unmount_fs(DISK_NAME);
    return 0;
}