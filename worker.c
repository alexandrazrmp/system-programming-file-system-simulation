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


void delete_file(const char* src, const char* tgt, const char* filename) {
    char full_src[PATH_MAX];
    char full_target[PATH_MAX];

    //create full path for source and target files 
    snprintf(full_src, sizeof(full_src), "%s/%s", src, filename);
    snprintf(full_target, sizeof(full_target), "%s/%s", tgt, filename); //name same as src

    if (unlink(full_target) == -1) {
        //mention error
        return;
    }
}

void sync_file(const char* src, const char* tgt, const char* filename) {
    char full_src[PATH_MAX];
    char full_target[PATH_MAX];

    //create full path for source and target files 
    snprintf(full_src, sizeof(full_src), "%s/%s", src, filename);
    snprintf(full_target, sizeof(full_target), "%s/%s", tgt, filename); //name same as src

    int fd_src = open(full_src, O_RDONLY);
    if (fd_src == -1) {
        //mention error
        return;
    }
    int fd_tgt = open(full_target, O_WRONLY | O_CREAT | O_TRUNC, 0644); //open target file for overwriting or create for writing
    if (fd_tgt == -1) {
        //mention error
        close(fd_src);
        return;
    }

    //do the actual copying

    ssize_t bytes_written;
    ssize_t bytes_read;
    char buffer[4096];
    while (1) {     //use a buffer of 4096 bytes to read from source file nd write to target file
        bytes_read = read(fd_src, buffer, sizeof(buffer));
        if (bytes_read == 0) break; //EOF, done reading

        if (bytes_read == -1) {
            //mention error
            break;
        }

        bytes_written = write(fd_tgt, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            //mention error
            break;
        }

    }
    close(fd_src);
    close(fd_tgt);
    return;
}



void op_all(const char* src, const char* tgt,  const char* op) {
    DIR * src_dir = opendir(src);
    DIR * tgt_dir = opendir(tgt);
    if (src_dir == NULL) return;    //source directory must exist
    struct dirent *file;

    while ((file = readdir(src_dir)) != NULL) {   //while there are files in the source directory
        if (file->d_ino == 0) continue; //skip empty entries
        //copy the file from src to tgt

        char* filename = file->d_name;
        if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) continue;

        if (strcmp(op, "DELETED") == 0) 
            delete_file(src, tgt, filename); //delete the file from the target directory
        else if (strcmp(op, "ADDED") == 0 || strcmp(op, "MODIFIED") == 0 || strcmp(op, "FULL") == 0)
            sync_file(src, tgt, filename); //sync the file from the source directory to the target directory
        else {}//mention error
        //move on to the next file
    }
    closedir(src_dir);
    closedir(tgt_dir);
    return;
}



int main(int argc, char* argv[]) {  //assuming the worker is called by fss_manager (correctly)
    char* src = argv[1];
    char* tgt = argv[2];
    char* filename = argv[3];
    char* operation = argv[4];

    printf("Worker running: src=%s tgt=%s\n", src, tgt);
    fflush(stdout);

    if (strcmp(filename, "ALL") == 0) {
        op_all(src, tgt, operation); //sync all files in the source directory
    }
    else {
        if (strcmp(operation, "ADDED") == 0) {
            sync_file(src, tgt, filename); //sync the file from the source directory to the target directory
        } else if (strcmp(operation, "FULL") == 0){ 
            //mention error
        } else if (strcmp(operation, "DELETED") == 0) {
            delete_file(src, tgt, filename); //delete the file from the target directory
        } else if (strcmp(operation, "MODIFIED") == 0) {
            sync_file(src, tgt, filename); //sync the file from the source directory to the target directory
        }
    }

    return 0;
}