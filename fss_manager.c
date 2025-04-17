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

#define READ 0
#define WRITE 1
#define DEFAULT_WORKER_LIMIT 5

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
    WorkerQueue * temp = worker_queue->next;
    free (worker_queue);
    worker_queue = temp;
}


void parse_config_file(FILE* file) {    //const
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


//clean up previous files


    char* log_file_ = NULL;
    char* config_file_ = NULL;
    int worker_limit = DEFAULT_WORKER_LIMIT;

    // Parse arguments
    int opt;
    while ((opt = getopt(argc, argv, "l:c:n:")) != -1) {
        switch (opt) {
            case 'l': log_file_ = optarg; break;
            case 'c': config_file_ = optarg; break;
            case 'n': worker_limit = atoi(optarg); break;
            default:
                fprintf(stderr, "Usage: %s -l <logfile> -c <config_file> [-n <worker_limit>]\n", argv[0]);
                exit(1);
        }
    }

    if (!log_file_ || !config_file_) {
        fprintf(stderr, "Input error\n");
        exit(1);
    }

    //cast file pointer to FILE*
    FILE* log_file = fopen(log_file_, "a");
    FILE* config_file = fopen(config_file_, "r");

    //named pipes
    if ( mkfifo ( "fss_in" , 0666) == -1 ) {
        if ( errno != EEXIST ) {
            perror ( "failed to create named pipe" ) ;
            exit (6) ;
        }
    }

    if ( mkfifo ( "fss_out" , 0666) == -1 ) {
        if ( errno != EEXIST ) {
            perror ( "failed to create named pipe" ) ;
            exit (6) ;
        }
    }

    //read config file
    parse_config_file(config_file);
    
    sync_info_mem_store* current = sync_list;
    for (int i = 0; i < worker_limit; i++) {
        if (current == NULL) {
            break;
        }
        //must change to immidiate fork without queue
        queue_push(i, current->source_dir, current->target_dir); // using the queue in the beginning as well
        current = current->next; // move to the next entry  
    }


    printf("FSS Manager started with a limit of %d workers.\n", worker_limit);


    printf("SyncList : %s, %s\n", sync_list->source_dir, sync_list->target_dir);                //to delete
    printf("WorkerQueue : %s, %s\n", worker_queue->source_dir, worker_queue->target_dir); //to delete


    int fd_in = open("fss_in", O_RDONLY);
    if (fd_in < 0) {
        perror("failed to open fss_in");
        fclose(log_file);
        exit(1);
    }

    int fd_out = open("fss_out", O_WRONLY);
    if (fd_out < 0) {
        perror("failed to open fss_out");
        fclose(log_file);
        close(fd_in);
        exit(1);
    }

    char input[MAX_LINE];
    char response[MAX_LINE];

//TESTTTTTTTTTTTTTT

    while (1) {
        ssize_t bytes = read(fd_in, input, sizeof(input) - 1);
        if (bytes > 0) {
            input[bytes] = '\0';
            printf("MANAGER received input %s\n", input);  // Parse and handle here
        }


        snprintf(response, sizeof(response), "all good\n"); //test

        if (write(fd_out, response, strlen(response)) < 0) {
            perror("MANAGER failed to write to fss_out");
            break;
        }

    }


    //loop to check for flags in fss_in and fork workers
    while (1) {
break;
        //if there is a worker process in the queue

        int pipeW[2];
        if (pipe(pipeW) == -1) {
            perror("pipe");
            exit(1);
        }
        
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) { // child process
            
//close read
//use open

        } else if (pid > 0){ // parent process


        }
break;
    }

//wait for the child process to finish

    return 0;
}
