/**
 * password_cracker
 * CS 341 - Fall 2024
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "cracker2.h"
#include "thread_status.h"

#define DEFAULT_THREADS 4

void usage() {
    fprintf(stderr, "\n  cracker2 [thread_count] < <password_file>\n\n");
    exit(1);
}

int main(int argc, char **argv) {
    size_t thread_count = DEFAULT_THREADS;

    if (argc > 1) {
        if (1 != sscanf(argv[1], "%lu", &thread_count) || thread_count < 1)
            usage();
    }

    // capture ctrl-c
    signal(SIGINT, threadStatusPrint);

    // student code called here
    int ret = start(thread_count);

    return ret;
}
