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
#include <dirent.h>



#define MAX_LINE 1024

void sync_all(const char* src, const char* tgt) {
    DIR * src_dir = opendir(src);
    DIR * tgt_dir = opendir(tgt);
    if (src_dir == NULL) return;
    struct dirent *file = readdir(src_dir);
    while (file != NULL) {
        //copy the file from src to tgt
        int fd_src = open(src, O_RDONLY);
        if (fd_src == -1) {
            //mention error////////////
            file = readdir(src_dir);
            continue;
        }
        int fd_tgt = open(tgt, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_tgt == -1) {
            perror("open(tgt)");
            close(fd_src);
            file = readdir(src_dir);
            continue;
        }



        file = readdir(src_dir);

    }
    closedir(src_dir);
    closedir(tgt_dir);
}









int main(int argc, char* argv[]) {  //assuming the worker is called by fss_manager (correctly)
    char* src = argv[1];
    char* tgt = argv[2];
    char* filename = argv[3];
    char* operation = argv[4];

    printf("Worker running: src=%s tgt=%s\n", src, tgt);
    fflush(stdout);

    if (strcmp(operation, "ADDED") == 0) {

    } else if (strcmp(operation, "FULL") == 0){ 
        sync_all(src, tgt); //sync all files in the source directory
    } else if (strcmp(operation, "DELETED") == 0) {
        //delete the entry from the list
        //delete_sync_entry(src, tgt);
    } else if (strcmp(operation, "MODIFIED") == 0) {
        //modify the entry in the list
        //modify_sync_entry(src, tgt);
    }





    
    return 0;
}