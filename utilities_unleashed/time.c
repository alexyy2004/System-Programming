/**
 * utilities_unleashed
 * CS 341 - Fall 2024
 */

 /**
  * In this lab, you will be implementing time.

time – run a program and report how long it took
So if a user enters:

./time sleep 2
then time will run sleep with the argument 2 and print how long it took in seconds:

sleep 2 took 2.002345 seconds
For more examples, you can play with Linux’s builtin time command by typing time YOURCOMMAND (time ls -l, for example) in your terminal. Be sure to add ./ to the beginning (or use the full path to your time executable file if you are in another directory), otherwise the builtin time will be called.

We’ve also provided a test executable to run basic tests on your time implementation. Note that although these tests are similar to those that will be run on the autograder they are not identical, so passing locally does not guarantee you will receive full credit. It is still your responsibility to ensure you have functional code.

Note that we only care about wall-clock time, and we recommend using clock_gettime with CLOCK_MONOTONIC.

Pro tip: 1 second == 1,000,000,000 nanoseconds.

Nota bene:
You may not use the existing time program.
You must use fork, exec, and wait (no other solutions will be accepted).
If the child process does not terminate successfully (where its exit status is non-zero), you should exit with status 1 without printing the time.
We will only run time with one program.
The commands we will run can take any number of arguments.
Do your time computations with double-precision floating pointer numbers (double) rather that single-precision (float).
We have provided functions in format.h that we expect you to use wherever appropriate.
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