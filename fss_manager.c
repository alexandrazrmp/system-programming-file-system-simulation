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

void parse_config_file(const char* config_path) {
    FILE* file = fopen(config_path, "r");
    if (!file) {
        printf("Errno is %d\n", errno);
        perror("fopen config_file");
        exit(1);
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file)) {
        char src[PATH_MAX], tgt[PATH_MAX];
        if (sscanf(line, "%s %s", src, tgt) == 2) {
            add_sync_entry(src, tgt);
        }
    }

    fclose(file);
}


//console to be made


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

    if (!log_file || !config_file) { //errno add
        fprintf(stderr, "Missing required arguments.\n");
        exit(1);
    }

    //named pipes
    mkfifo("fss_in", 0666);
    mkfifo("fss_out", 0666);

    //read config file
    parse_config_file(config_file);

    //initial sync



    printf("FSS Manager started with a limit of %d workers.\n", worker_limit);

    //loop to check for flags in fss_in
    while (1) {
        





    }

    return 0;
}
