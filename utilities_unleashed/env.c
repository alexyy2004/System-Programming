/**
 * utilities_unleashed
 * CS 341 - Fall 2024
 */
 #include <stdio.h>
 #include <stdint.h>
 #include <unistd.h>
 #include <stdlib.h>
 #include <sys/types.h>
 #include <sys/wait.h>
 #include <string.h>
 #include <ctype.h>
 #include "format.h"
 /**
    In this lab, you will be implementing a special version of env.
  */
int main(int argc, char *argv[]) {
    if (argc < 4) {
        print_env_usage();
    }

    int i = 1;
    while (i < argc && strchr(argv[i], '=') != NULL) {
        i++;
    }

    if (i == argc || strcmp(argv[i], "--") != 0) {
        print_env_usage();
    }

    char *cmd = argv[i + 1];
    char *args[argc - i];
    for (int j = 0; j < argc - i; j++) {
        args[j] = argv[j + i];
    }
    args[argc - i] = NULL;

    char *envp[argc];
    for (int j = 1; j < i; j++) {
        envp[j - 1] = argv[j];
    }
    envp[i - 1] = NULL;

    pid_t pid = fork();
    if (pid < 0) {
        print_fork_failed();
    } else if (pid == 0) {
        if (execvp(cmd, args) < 0) {
            print_exec_failed();
        }
    } else {
        int status;
        wait(&status);
    }
    
    return 0;
}
