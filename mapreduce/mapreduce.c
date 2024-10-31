// /**
//  * mapreduce
//  * CS 341 - Fall 2024
//  */
// #include "utils.h"

// int main(int argc, char **argv) {
//     // Create an input pipe for each mapper.

//     // Create one input pipe for the reducer.

//     // Open the output file.

//     // Start a splitter process for each mapper.

//     // Start all the mapper processes.

//     // Start the reducer process.

//     // Wait for the reducer to finish.

//     // Print nonzero subprocess exit codes.

//     // Count the number of lines in the output file.

//     return 0;
// }

/**
 * mapreduce
 * CS 341 - Fall 2024
 * inspired flowchat with jieshuh2, collab with yanzelu2
 */

#include "utils.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

/* 
 * input format:
 * ./mapreduce <input_file> <output_file> <mapper_executable> <reducer_executable> <mapper_count>
 */

//partly debugged with gpt
int main(int argc, char **argv) {
    if (argc != 6) {
        print_usage();
        return 0;
    }

    // Parse the number of mappers
    int mapper_count = atoi(argv[5]);
    if (mapper_count <= 0) {
        print_usage();
        return 0;
    }


    // Allocate pipes for splitters and mappers
    int **mapper_pipes = (int **)calloc(mapper_count, sizeof(int *));
    for (int i = 0; i < mapper_count; i++) {
        mapper_pipes[i] = (int *)calloc(2, sizeof(int));
        if (pipe2(mapper_pipes[i], O_CLOEXEC) == -1) {
            perror("pipe");
            return 1;
        }
    }

    // Pipe for communication between mappers and reducer
    int reducer_pipe[2];
    if (pipe2(reducer_pipe, O_CLOEXEC) == -1) {
        perror("pipe");
        return 1;
    }

    // Open the output file
    int output_fd = open(argv[2], O_CREAT | O_TRUNC | O_WRONLY, S_IWUSR | S_IRUSR);
    if (output_fd == -1) {
        perror("open");
        return 1;
    }

    // Array for storing PIDs of splitters and mappers
    pid_t *splitter_pids = (pid_t *)calloc(mapper_count, sizeof(pid_t));
    pid_t *mapper_pids = (pid_t *)calloc(mapper_count, sizeof(pid_t));

    // Start splitter processes
    for (int i = 0; i < mapper_count; i++) {
        splitter_pids[i] = fork();
        if (splitter_pids[i] == 0) {
            // Child process (splitter)
            close(mapper_pipes[i][0]);  // Close read end
            dup2(mapper_pipes[i][1], STDOUT_FILENO);  // Redirect stdout to pipe write end
            char index[3];
            snprintf(index, sizeof(index), "%d", i);
            execlp("./splitter", "./splitter", argv[1], argv[5], index, (char*)NULL);
            perror("execlp fail");
            exit(-1);
        }
        close(mapper_pipes[i][1]);  
    }

    // Start mapper processes
    for (int i = 0; i < mapper_count; i++) {
        mapper_pids[i] = fork();
        if (mapper_pids[i] == 0) {
            // Child process (mapper)
            dup2(mapper_pipes[i][0], 0);
            dup2(reducer_pipe[1], 1);    // Redirect stdout to reducer pipe write end
            execlp(argv[3], argv[3], (char*)NULL);
            perror("execlp mapper");
            exit(1);
        }
        close(mapper_pipes[i][0]);  // Close read end in parent
    }
    close(reducer_pipe[1]);  // Close write end of reducer pipe in parent

    // Start the reducer process
    pid_t reducer_pid = fork();
    if (reducer_pid == 0) {
        // Child process (reducer)
        dup2(reducer_pipe[0], STDIN_FILENO);         
        dup2(output_fd, STDOUT_FILENO);             
        execlp(argv[4], argv[4], (char*)NULL);
        perror("execlp reducer");
        exit(1);
    }
    close(reducer_pipe[0]);  
    close(output_fd);        

    // Wait for all splitters to complete
    for (int i = 0; i < mapper_count; i++) {
        int splitter_status;
        waitpid(splitter_pids[i], &splitter_status, 0);
        if (WIFEXITED(splitter_status) && WEXITSTATUS(splitter_status) != 0) {
            print_nonzero_exit_status("/splitter", WEXITSTATUS(splitter_status));
        }
    }

    // Wait for all mappers to complete
    for (int i = 0; i < mapper_count; i++) {
        int mapper_status;
        waitpid(mapper_pids[i], &mapper_status, 0);
        if (WIFEXITED(mapper_status) && WEXITSTATUS(mapper_status) != 0) {
            print_nonzero_exit_status(argv[3], WEXITSTATUS(mapper_status));
        }
    }

    // Wait for reducer to complete
    int reducer_status;
    waitpid(reducer_pid, &reducer_status, 0);
    if (WIFEXITED(reducer_status) && WEXITSTATUS(reducer_status) != 0) {
        print_nonzero_exit_status(argv[4], WEXITSTATUS(reducer_status));
    }

    // Print the number of lines in the output file
    print_num_lines(argv[2]);

    // Free allocated resources
    for (int i = 0; i < mapper_count; i++) {
        free(mapper_pipes[i]);
    }
    free(mapper_pipes);
    free(splitter_pids);
    free(mapper_pids);

    return 0;
}


