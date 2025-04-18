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


typedef struct WorkerQueue {        // to fix
    int worker_id;
    char source_dir[PATH_MAX];
    char target_dir[PATH_MAX];
    struct WorkerQueue* next;
} WorkerQueue;



void queue_push(WorkerQueue *worker_queue, int worker_id, const char* src, const char* tgt);
void queue_pop(WorkerQueue *worker_queue) ;