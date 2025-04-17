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


#define DEFAULT_WORKER_LIMIT 5
//should worker limit be global?

#define MAX_LINE 1024

//linked list to store sync info
typedef struct sync_info_mem_store_struct {
    char source_dir[PATH_MAX];
    char target_dir[PATH_MAX];
    int status;
    time_t last_sync_time;
    int active;
    int error_count;
    struct sync_info_mem_store_struct* next;
} sync_info_mem_store;

sync_info_mem_store* sync_list = NULL;

typedef struct WorkerQueue {        // to fix
    int worker_id;
    char source_dir[PATH_MAX];
    char target_dir[PATH_MAX];
    struct WorkerQueue* next;
} WorkerQueue;

WorkerQueue* worker_queue = NULL;


//add entry to linked list
void add_sync_entry(const char* src, const char* tgt) {
    sync_info_mem_store* entry = malloc(sizeof(sync_info_mem_store));
    strcpy(entry->source_dir, src);
    strcpy(entry->target_dir, tgt);
    entry->last_sync_time = 0;
    entry->active = 0;
    entry->error_count = 0;
    entry->next = sync_list;
    sync_list = entry;
}


void queue_push(int worker_id, const char* src, const char* tgt) {
    WorkerQueue* new_worker = malloc(sizeof(WorkerQueue));
    new_worker->worker_id = worker_id;
    strcpy(new_worker->source_dir, src);
    strcpy(new_worker->target_dir, tgt);
    new_worker->next = worker_queue;
    worker_queue = new_worker;
}

void queue_pop() {

    if (worker_queue == NULL) {
        return;
    }
    worker_queue = worker_queue->next;
}


void parse_config_file(const char* config_path) {
    FILE* file = fopen(config_path, "r");
    if (!file) {
        perror("fopen config_file");
        exit(1);
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file)) {
        char src[PATH_MAX], tgt[PATH_MAX];
        if (sscanf(line, " ( %[^,] , %[^)] ) ", src, tgt)== 2) {
            add_sync_entry(src, tgt);
        }
    }

    fclose(file);
}


int main(int argc, char* argv[]) {
    char* log_file = NULL;
    char* config_file = NULL;
    int worker_limit = DEFAULT_WORKER_LIMIT;

    // Parse arguments
    int opt;
    while ((opt = getopt(argc, argv, "l:c:n:")) != -1) {
        switch (opt) {
            case 'l': log_file = optarg; break;
            case 'c': config_file = optarg; break;
            case 'n': worker_limit = atoi(optarg); break;
            default:
                fprintf(stderr, "Usage: %s -l <logfile> -c <config_file> [-n <worker_limit>]\n", argv[0]);
                exit(1);
        }
    }

    if (!log_file || !config_file) {
        fprintf(stderr, "Missing required arguments.\n");
        exit(1);
    }

    //named pipes
    if ( mkfifo ( "fss_in" , 0666) == -1 ) {
        if ( errno != EEXIST ) {
            perror ( " receiver : mkfifo " ) ;
            exit (6) ;
        }
    }

    if ( mkfifo ( "fss_out" , 0666) == -1 ) {
        if ( errno != EEXIST ) {
            perror ( " receiver : mkfifo " ) ;
            exit (6) ;
        }
    }

    //read config file
    parse_config_file(config_file);
    //call workers


    printf("FSS Manager started with a limit of %d workers.\n", worker_limit);
printf("SyncList : %s, %s\n", sync_list->source_dir, sync_list->target_dir);
    //loop to check for flags in fss_in and fork workers
    while (1) {
        break;

    }

    return 0;
}
