#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "disk.h"
#include "fs.h"

#define DISK_NAME "vdisk"

int main(void){
    
    int result = make_fs(DISK_NAME);   

    if (result == 0) {
        printf("SUCCESS: make_fs finished perfectly.\n");
    } else {
        printf("ERROR: make_fs returned -1 and failed.\n");
    }
    
    return 0;

}