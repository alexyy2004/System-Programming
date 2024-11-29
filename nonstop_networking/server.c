/**
 * nonstop_networking
 * CS 341 - Fall 2024
 */
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


void signal_handler() {
    // ignore SIGPIPE
}

void sigint_handler() {
    
}

int main(int argc, char **argv) {
    // good luck!
    signal(SIGPIPE, signal_handler);
	if (argc != 2) {
        print_server_usage();
        exit(1);
	}
	struct sigaction sa;
    sa.sa_handler = sigint_handler;    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}
