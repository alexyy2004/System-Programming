/**
 * utilities_unleashed
 * CS 341 - Fall 2024
 */

 /**
  * In this lab, you will be implementing time.
  */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "format.h"


int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_time_usage();
    }
    struct timespec start, end;
    double duration;
    clock_gettime(CLOCK_MONOTONIC, &start);
    pid_t pid = fork();

    if (pid < 0) { // fork failed
        print_fork_failed();
    } else if (pid == 0) { // child process
        if (execvp(argv[1], argv + 1) < 0) {
            print_exec_failed();
        }
    } else { // parent process
        int status;
        wait(&status);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            clock_gettime(CLOCK_MONOTONIC, &end);
            duration = (end.tv_sec - start.tv_sec) + (double) (end.tv_nsec - start.tv_nsec) / ((double) 1000000000);
            display_results(argv, duration);
        } else {
            exit(1);
        }
    }
    return 0;

    // return 0;
    // struct timespec start, end;
    // double duration;
    // clock_gettime(CLOCK_MONOTONIC, &start);
    // sleep(1);
    // clock_gettime(CLOCK_MONOTONIC, &end);
    // duration = (end.tv_sec - start.tv_sec) + (double) (end.tv_nsec - start.tv_nsec) / (double) BILLION;
    // display_results(argv, duration);
    // return 0;
}