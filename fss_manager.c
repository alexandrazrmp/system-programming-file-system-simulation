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

#include "List.h"
#include "Queue.h"

#define DEFAULT_WORKER_LIMIT 5

#define MAX_LINE 1024

sync_info_mem_store* sync_list = NULL;

WorkerQueue* worker_queue = NULL;

void parse_config_file(FILE* file) {    //const
    if (!file) {
        perror("fopen config_file");
        exit(1);
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file)) {
        char src[PATH_MAX], tgt[PATH_MAX];
        if (sscanf(line, " ( %[^,] , %[^)] ) ", src, tgt)== 2) {
            add_sync_entry(sync_list, src, tgt);
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
                fprintf(stderr, "Please give input in the form ./fss_manager -l <logfile> -c <config_file> [-n <worker_limit>]\n");
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
        queue_push(worker_queue, i, current->source_dir, current->target_dir); // using the queue in the beginning as well
        current = current->next; // move to the next entry  
    }


    printf("FSS Manager started with a limit of %d workers.\n", worker_limit);


    // printf("SyncList : %s, %s\n", sync_list->source_dir, sync_list->target_dir);                //to delete
    // printf("WorkerQueue : %s, %s\n", worker_queue->source_dir, worker_queue->target_dir); //to delete


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

        //handle input assuming it is always valid as it is from the console
        char * instruction, *arg1, *arg2;
        instruction = strtok(input, " ");
        arg1 = strtok(NULL, " ");
        arg2 = strtok(NULL, " ");
        if (strcmp(instruction, "shutdown") == 0) {
            printf("MANAGER received shutdown instruction\n");
            //handle shutdown (wait for all workers to finish, also those in queue)
            //shutdown();

            //write to log file
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            char timebuf[64];
            strftime(timebuf, sizeof(timebuf), "[%Y-%m-%d %H:%M:%S]", t);   //get the corerct time and format
        
            fprintf(log_file, "%s Command shutdown", timebuf);   //write to log file

            break;
        } else if (strcmp(instruction, "sync") == 0) { //use a worker to sync source directory arg1 to target directory
            //find target directory in sync_list

            //fork and exec a worker process to sync source to target if we have not reached the limit

            //if the limit is reached, add to queue

            //write to log file
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            char timebuf[64];
            strftime(timebuf, sizeof(timebuf), "[%Y-%m-%d %H:%M:%S]", t);   //get the corerct time and format
        
            fprintf(log_file, "%s Command sync %s\n", timebuf, arg1);   //write to log file
            fflush(log_file); // flush to ensure it's written immediately
        } else if (strcmp(instruction, "cancel") == 0) {

            //remove the entry from the sync_list and free the memory (maintain it in the queue)


            //write to log file
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            char timebuf[64];
            strftime(timebuf, sizeof(timebuf), "[%Y-%m-%d %H:%M:%S]", t);   //get the corerct time and format
        
            fprintf(log_file, "%s Command cancel %s\n", timebuf, arg1);   //write to log file
            fflush(log_file); // flush to ensure it's written immediately

        } else if (strcmp(instruction, "status") == 0) {

            //check the status of the sync_list and print it to the console

            //write to log file

            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            char timebuf[64];
            strftime(timebuf, sizeof(timebuf), "[%Y-%m-%d %H:%M:%S]", t);   //get the corerct time and format
        
            fprintf(log_file, "%s Command status %s\n", timebuf, arg1);   //write to log file
            fflush(log_file); // flush to ensure it's written immediately
        } else if (strcmp(instruction, "add") == 0) {
            //add the entry to the sync_list (source and target directories)
            //write to log file
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            char timebuf[64];
            strftime(timebuf, sizeof(timebuf), "[%Y-%m-%d %H:%M:%S]", t);   //get the corerct time and format
        
            fprintf(log_file, "%s Command sync %s %s\n", timebuf, arg1, arg2);   //write to log file
            fflush(log_file); // flush to ensure it's written immediately
        }


        snprintf(response, sizeof(response), "all good\n"); //test

        if (write(fd_out, response, strlen(response)) < 0) {
            perror("MANAGER failed to write to fss_out");
            break;
        }

    }

    close(fd_in);
    close(fd_out);
    fclose(log_file);

    //clean up
    while (worker_queue != NULL) {
        queue_pop(worker_queue);
        //handle sync
    }
    while (sync_list != NULL) {
        delete_sync_entry(sync_list, sync_list->source_dir);
    }

    //wait for all child processes to finish


    return 0;
}