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

void worker(const char* src, const char* tgt, const char* filename, const char* op) {

    int src_ = open(src, O_RDONLY);
    if (src_ < 0) {
        perror("failed to open source file");
        return;
    }
    int tgt_ = open(tgt, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (tgt_ < 0) {
        perror("failed to open target file");
        close(src_);
        return;
    }
    char buffer[MAX_LINE];
    ssize_t bytes;



    return;

}