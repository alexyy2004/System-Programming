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
    
    pid_t pid = fork();
    if (pid < 0) {
        print_fork_failed();
    } else if (pid == 0) { // child
        int i = 1;
        while (i < argc && argv[i] != NULL) {
            if (strcmp(argv[i], "--") == 0) { // command solve
                execvp(argv[i + 1], &argv[i + 1]);
                print_exec_failed();
            } else { // env solve
                char *key = strtok(argv[i], "=");
                char *value = strtok(NULL, "=");
                if (value == NULL) {
                    print_env_usage();
                }
                if (key == NULL) {
                    print_env_usage();
                }

                // check key is valid
                char *ptr_k = key;
                while (*ptr_k) {
                    if (!isalpha(*ptr_k) && !isdigit(*ptr_k) && *ptr_k != '_') {
                        print_env_usage();
                    }
                    ptr_k++;
                }

                // check value is reference or value
                if (value[0] == '%') {
                    value = getenv(value + 1);
                    if (value == NULL) {
                        print_environment_change_failed();
                    }
                }

                // check value is valid
                char *ptr_v = value;
                while (*ptr_v) {
                    if (!isalpha(*ptr_v) && !isdigit(*ptr_v) && *ptr_v != '_') {
                        print_env_usage();
                    }
                    ptr_v++;
                }


                if (setenv(key, value, 1) < 0) {
                    print_environment_change_failed();
                } 
            }
            i += 1;
        }
        print_env_usage();
    } else { // parent
        int status;
        waitpid(pid, &status, 0);
    }
    
    return 0;
}

