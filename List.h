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

//add entry to linked list
void add_sync_entry(sync_info_mem_store** sync_list, const char* src, const char* tgt) ;

//check if entry exists in linked list
sync_info_mem_store* exists_sync_entry(sync_info_mem_store* sync_list, const char* src, const char* tgt) ;

//delete entry from linked list using source directory
void delete_sync_entry(sync_info_mem_store** sync_list, const char* src);