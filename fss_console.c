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
#define MAX_LINE 1024

int main(int argc, char *argv[]) {

    if (argc != 3 || strcmp(argv[1], "-l") != 0) {
        fprintf(stderr, "Please give input in the form ./fss_console -l <console-logfile>");
        return 1;
    }

    char *logfile = argv[2];
    FILE *log_file = fopen(logfile, "a");
    if (log_file == NULL) {
        perror("Failed to open log file");
        return 1;
    }

//write to console log file


    int fd_in = open("fss_in", O_WRONLY);
    if (fd_in < 0) {
        perror("failed to open fss_in");
        fclose(log_file);
        exit(1);
    }

    int fd_out = open("fss_out", O_RDONLY);
    if (fd_out < 0) {
        perror("failed to open fss_out");
        fclose(log_file);
        close(fd_in);
        exit(1);
    }

    char input[MAX_LINE];
    char response[MAX_LINE];

    while (1) {
        printf("Insert instruction:\n");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) break;

        input[strcspn(input, "\n")] = '\0'; // Turn newline into null terminator
        if (strcmp(input, "exit") == 0) {

//hadle manager and workers

            printf("Exiting console.\n");
            break;
        }

        if (write(fd_in, input, strlen(input)) < 0) {
            perror("CONSOLE failed to write to fss_in");
            break;
        }

        // Wait and read response from manager
        ssize_t n = read(fd_out, response, sizeof(response) - 1);
        if (n > 0) {
            response[n] = '\0';
            printf("CONSOLE got back response: %s\n", response);
        }
    }

    close(fd_in);
    close(fd_out);



    return(0);
}
