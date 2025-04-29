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
#include <time.h>
#include <bits/getopt_core.h>
#include <sys/inotify.h>
#include <sys/select.h>

#include "List.h"
#include "Queue.h"

#define DEFAULT_WORKER_LIMIT 5

#define MAX_LINE 1024

int *worker_array; //will be initialized later when worker limit is set
int worker_count = 0; //number of workers currently running
int worker_limit = 0; //maximum number of workers allowed

sync_info_mem_store* sync_list = NULL;

WorkerQueue* worker_queue = NULL;


int inotify_fd;

void setup_inotify() {
    inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd < 0) {
        perror("inotify_init1 failed");
        exit(1);
    }
}

void handle_inotify_events() {
    char buffer[MAX_LINE];
    ssize_t length = read(inotify_fd, buffer, sizeof(buffer));
    if (length <= 0) return; //no event found

    for (char *ptr = buffer; ptr < buffer + length; ) {
        struct inotify_event *event = (struct inotify_event*)ptr;
        ptr += sizeof(struct inotify_event) + event->len;

        //find which sync entry this watch descriptor belongs to
        sync_info_mem_store *entry = sync_list;
        while (entry && entry->wd != event->wd) entry = entry->next;
        if (!entry) continue;

        //find operation based on event mask
        const char *operation = NULL;
        if (event->mask & IN_CREATE ) operation = "ADDED";
        else if (event->mask & IN_MODIFY) operation = "MODIFIED";
        else if (event->mask & IN_DELETE) operation = "DELETED";
        else continue; //ignore other events

        if (worker_count < worker_limit) {
            start_worker(entry->source_dir, entry->target_dir, event->name, operation);
        } else {
            worker_queue = queue_push(worker_queue, entry->source_dir, entry->target_dir, event->name, operation);
        }
    }
}



//add directory watch and return the watch descriptor
int add_directory_watch(const char *path) {
    int wd = inotify_add_watch(inotify_fd, path, IN_CREATE | IN_MODIFY | IN_DELETE );
    if (wd == -1) {
        perror("inotify_add_watch");
    }
    return wd;
}

//signal handler for SIGCHLD
volatile sig_atomic_t child_exited = 0; //flag to start a queued worker

void sigchld_handler(int sig) {
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        //remove pid from worker_array
        for (int i = 0; i < worker_count; ++i) {
            if (worker_array[i] == pid) {
                //shift the remaining child processes to the left
                for (int j = i; j < worker_count - 1; ++j) {
                    worker_array[j] = worker_array[j + 1];
                }
                worker_count--;
                break;
            }
        }
    }
    child_exited = 1;
}

void parse_config_file(FILE* file) {    //const
    if (!file) {
        perror("fopen config_file");
        exit(1);
    }
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file)) {
        char src[PATH_MAX] = {0}, tgt[PATH_MAX] = {0};
    
        if (sscanf(line, " ( %[^,] , %[^)] ) ", src, tgt) == 2) {
            sync_list = add_sync_entry(&sync_list, src, tgt);
            sync_info_mem_store* current = exists_sync_entry(sync_list, src, tgt);
            current->wd = add_directory_watch(current->source_dir); // Monitor source directory
        }
    }
    

    fclose(file);
}


void start_worker(const char* src, const char* tgt, const char* filename, const char* operation) {
    //create read-write named pipe for each worker
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe creation failed");
        return;
    }

    sync_info_mem_store* cur = exists_sync_entry(sync_list, src, tgt);
    cur->status = SYNCING; //mark as syncing

    pid_t pid = fork();
    if (pid == 0) { // child process

        close(pipefd[0]); // close read end of the pipe
        dup2(pipefd[1], STDOUT_FILENO); // redirect stdout to the write end of the pipe
        close(pipefd[1]); // close write end of the pipe

        char* const args[] = {"./worker", (char *)src, (char *)tgt, (char *)filename, (char *)operation, NULL};
        execv(args[0], args);

        perror("execv failed");
        exit(1);

    } else if (pid < 0) {
        perror("fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
    } else { // parent process
        printf("Started worker with PID %d for %s to %s\n", pid, src, tgt);
        worker_array[worker_count++] = pid; //add to worker array
        close(pipefd[1]);

        //read from the pipe
        char buffer[MAX_LINE];
        ssize_t bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0'; // null-terminate the string
            printf("%s", buffer); // print the report as is
        } else if (bytes_read == 0) {
            printf("No bytes in the pipe\n");
        } else {
            perror("read failed");
        }
        close(pipefd[0]); // close read

    }
    waitpid(pid, NULL, 0); // wait for the child process to finish
    cur->status = NOT_SYNCING; //mark as not syncing


    return;
}


int main(int argc, char* argv[]) {


//clean up previous files


    char* log_file_ = NULL;
    char* config_file_ = NULL;
    worker_limit = DEFAULT_WORKER_LIMIT;

    // Parse arguments
    int opt;
    while ((opt = getopt(argc, argv, "l:c:n:")) != -1) {
        switch (opt) {
            case 'l': log_file_ = optarg; break;
            case 'c': config_file_ = optarg; break;
            case 'n': if (atoi(optarg)>0) worker_limit = atoi(optarg); break;
            default:
                fprintf(stderr, "Please give input in the form ./fss_manager -l <logfile> -c <config_file> -n <worker_limit>\n");
                exit(1);
        }
    }

    worker_array = malloc(sizeof(int) * worker_limit); //initialize worker array

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

    printf("FSS Manager started with a limit of %d workers.\n", worker_limit);

    //read config file and add entries to sync_list
    parse_config_file(config_file);

    //do the initial sync
    sync_info_mem_store* current = sync_list;
    for (int i = 0; i < worker_limit; i++) {
        if (current == NULL) break;
        start_worker(current->source_dir, current->target_dir, "ALL", "FULL"); //start worker process
        current = current->next; // move to the next entry
    }

    worker_queue = queue_create();

    //if there are more entries, add them to the queue
    while (current != NULL) {
        worker_queue = queue_push(worker_queue, current->source_dir, current->target_dir, "ALL", "FULL"); //add to queue
        current = current->next; // move to the next entry  
    }


//open pipes
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


    while (1) {
        //if a file is modified, start a worker


        //if a file is deleted, start a worker


        //if a file is added, start a worker





/*        
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(fd_in, &read_fds);
        FD_SET(inotify_fd, &read_fds);
    
        int max_fd = (fd_in > inotify_fd) ? fd_in : inotify_fd;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }
    
        if (FD_ISSET(inotify_fd, &read_fds)) {
            handle_inotify_events();
        }
    
        if (FD_ISSET(fd_in, &read_fds)) {

            ssize_t bytes = read(fd_in, input, sizeof(input) - 1);
            if (bytes > 0) {
                input[bytes] = '\0';
            }
            else if (bytes == 0) {
                printf("MANAGER got no instruction\n");
                break;
            } else {
                perror("MANAGER failed to read from fss_in");
                break;
            }
    
            //time to be used in the log file
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            char timebuf[64];
            strftime(timebuf, sizeof(timebuf), "[%Y-%m-%d %H:%M:%S]", t);   //get the corerct time and format
        
    
            //handle input assuming it is always valid as it is from the console
            char * instruction, *arg1, *arg2;
            instruction = strtok(input, " ");
            arg1 = strtok(NULL, " ");
            arg2 = strtok(NULL, " ");
            if (strcmp(instruction, "shutdown") == 0) {
                //print messages
                printf("%s Shutting down manager...\n", timebuf);
                printf("%s Waiting for all active workers to finish.\n", timebuf);
    
                //printing to be continued after "break"
    
                //write to log file
                fprintf(log_file, "%s Command shutdown", timebuf);   //write to log file
                fflush(log_file); // flush to ensure it's written immediately
                break;
            } else if (strcmp(instruction, "sync") == 0) { //use a worker to sync source directory arg1 to target directory
                
                fprintf(log_file, "%s Command sync %s\n", timebuf, arg1);   //write to log file
                fflush(log_file); // flush to ensure it's written immediately
                sync_info_mem_store *cur;
                if ((cur = exists_sync_entry(sync_list, arg1, NULL)) != NULL && cur->status != SYNCING) {
                    printf("%s Syncing directory: %s -> %s\n", timebuf, arg1, cur->target_dir);
                    if (worker_limit > worker_count) {
                        cur->active = 1; //mark as active
                        cur->last_sync_time = time(NULL); //update last sync time
                        cur->status = SYNCING; //mark as syncing
                        cur->error_count = 0; //reset error count
                        //start worker process
                        start_worker(arg1, cur->target_dir, "ALL", "FULL");
                    } else {
                        worker_queue = queue_push(worker_queue, arg1, cur->target_dir, "ALL", "FULL"); //add to queue
                    }
                }else {
                    printf("%s  Sync already in progress: %s\n", timebuf, arg1);
                }
    
            } else if (strcmp(instruction, "cancel") == 0) {
                //mark the entry from the sync_list as not active but maintain it in the queue
                sync_info_mem_store *cur = exists_sync_entry(sync_list, arg1, NULL);
                if (cur && cur->active) {
                    if (cur->status == SYNCING) {
                        //wait for the worker to finish
                        //then cancel
                    }
                    cur->active = 0;    //mark as not active
                    printf("%s Monitoring stopped for %s\n", timebuf, arg1);
                    fprintf(log_file, "%s Monitoring stopped for %s\n", timebuf, arg1);
                } else if (cur == NULL){
                    printf("%s Directory not monitored: %s\n", timebuf, arg1);
                }
                fflush(log_file);
            
            } else if (strcmp(instruction, "status") == 0) {
    
                //check the status of the sync_list and print it to the console
    
                sync_info_mem_store *cur = exists_sync_entry(sync_list, arg1, NULL);
                if (cur && cur->active) {
                    printf("Directory: %s\n", cur->source_dir);
                    printf("Target: %s\n", cur->target_dir);
                    printf("Last Sync: %ld\n", cur->last_sync_time);
                    printf("Errors: %d\n", cur->error_count);
                    printf("Status: Active\n");
                } else {
                    printf("%s Directory not monitored: %s\n", timebuf, arg1);
                }
    
                //write to log file
    
                fprintf(log_file, "%s Command status %s\n", timebuf, arg1);   //write to log file
                fflush(log_file); // flush to ensure it's written immediately
            } else if (strcmp(instruction, "add") == 0) {
                //add the entry to the sync_list (source and target directories)
                if (exists_sync_entry(sync_list, arg1, arg2) == NULL) {
                    sync_list = add_sync_entry(&sync_list, arg1, arg2);
                    sync_info_mem_store* current = exists_sync_entry(sync_list, arg1, arg2);
                    current->wd = add_directory_watch(current->source_dir); // Monitor source directory
                } else {
                    fprintf(stderr, "Entry already exists\n");
                }
    
    
                fprintf(log_file, "%s Command add %s %s\n", timebuf, arg1, arg2);   //write to log file
                fflush(log_file); // flush to ensure it's written immediately
            }
    
    
            snprintf(response, sizeof(response), "all good\n"); //test
    
            if (write(fd_out, response, strlen(response)) < 0) {
                perror("MANAGER failed to write to fss_out");
                break;
            }

        }
    
        // Handle SIGCHLD
        if (child_exited) {
            handle_exited_children();
            child_exited = 0;
        }
    }




    close(fd_in);
    close(fd_out);
    fclose(log_file);


*/



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //print  queue
    WorkerQueue* cur = worker_queue;
    while (cur != NULL) {
        printf("Queue: %s -> %s\n", cur->source_dir, cur->target_dir);
        cur = cur->next;
    }

    //print sync_list
    sync_info_mem_store* cur2 = sync_list;
    while (cur2 != NULL) {
        printf("Sync List: %s -> %s\n", cur2->source_dir, cur2->target_dir);
        cur2 = cur2->next;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////



            //handle shutdown wait for all workers to finish
    for (int i = 0; i < worker_count; i++) {
        kill(worker_array[i], SIGTERM); //send SIGTERM to all workers
    }            //wait for all child processes to finish

    //clean up
    while (worker_queue != NULL) {
        WorkerQueue * cur = queue_pop(&worker_queue);
        if (cur != NULL) {
            free(cur);
        }
        //handle sync
    }

    sync_info_mem_store *entry = sync_list;
    while (entry) {
        if (entry->wd != -1) {
            inotify_rm_watch(inotify_fd, entry->wd);
        }
        entry = entry->next;
    }
    close(inotify_fd);


    while (sync_list != NULL) {
        delete_sync_entry(&sync_list, sync_list->source_dir);
    }


    fflush(stdout); //flush to ensure it's written immediately

    //wait for all child processes to finish

    for (int i = 0; i < worker_count; i++) {
        waitpid(worker_array[i], NULL, 0);
    }
    free(worker_array);
    unlink("fss_in");
    unlink("fss_out");

    return 0;
}