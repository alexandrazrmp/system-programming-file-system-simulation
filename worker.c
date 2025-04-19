#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h> 
#include <limits.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
#include <signal.h>
#include <bits/getopt_core.h>
#include <time.h>



#define MAX_LINE 1024

int main(int argc, char* argv[]) {  //assuming the worker is called by fss_manager (correctly)
    char* src = argv[1];
    char* tgt = argv[2];
    char* filename = argv[3];
    char* operation = argv[4];

    printf("Worker running: src=%s tgt=%s\n", src, tgt);
    fflush(stdout);

    if (strcmp(operation, "ADDED") == 0) {
        //new pair added to the list
        //sync(src, tgt);
    } else if (strcmp(operation, "FULL") == 0){ 
        //full sync
        //sync(src, tgt);
    } else if (strcmp(operation, "DELETED") == 0) {
        //delete the entry from the list
        //delete_sync_entry(src, tgt);
    } else if (strcmp(operation, "MODIFIED") == 0) {
        //modify the entry in the list
        //modify_sync_entry(src, tgt);
    }





    
    return 0;
}