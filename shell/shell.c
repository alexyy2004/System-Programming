/**
 * shell
 * CS 341 - Fall 2024
 */

#include "format.h"
#include "shell.h"
#include "vector.h"
#include "sstring.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <ctype.h>
#include <dirent.h>

#define BUFFER_SIZE 1024
#define MAX_HISTORY 1000

typedef struct process {
    char *command;
    pid_t pid;
} process;

// Global variables for history and processes
static vector *history;
static vector *process_list;  // Vector to store active processes
static char *history_file = NULL;

// Function declarations
void shell_loop();
void execute_command(char **args);
int shell_cd(char **args);
void handle_logical_operators(char **args);
void add_to_history(const char *command);
char **parse_input(char *input);
void handle_signals();
void load_history(const char *filename);
void save_history(const char *filename);
void print_history_cmd();
void run_history_cmd(size_t index);
void run_prefix_cmd(const char *prefix);
void add_process(char *command, pid_t pid);
void remove_process(pid_t pid);

// Signal handler for SIGINT (Ctrl+C)
void handle_sigint(int sig) {
    // (void)sig;  // Avoid unused parameter warning
    // printf("\n");
    // print_prompt(getcwd(NULL, 0), getpid());  // Reprint prompt after signal
    // fflush(stdout);
    for (size_t i = 0; i < vector_size(process_list); i++) {
        process *prcss = (process *) vector_get(process_list, i);
        if ( prcss->pid != getpgid(prcss->pid) ){
        kill(prcss->pid, SIGKILL);
        // printf("finished foreground process: %d\n", prcss->pid);
        remove_process(prcss->pid);
        }
    }
}

// Main entry point for the shell
int shell(int argc, char *argv[]) {
    int opt;
    char *script_file = NULL;

    // Initialize history and process vectors
    history = string_vector_create();
    process_list = shallow_vector_create();  // Initialize process list

    // Parse optional arguments using getopt
    while ((opt = getopt(argc, argv, "h:f:")) != -1) {
        switch (opt) {
            case 'h':
                history_file = optarg;
                load_history(history_file);
                break;
            case 'f':
                script_file = optarg;
                break;
            default:
                print_usage();
                return EXIT_FAILURE;
        }
    }

    // If script file is provided but not found, print error and exit
    if (script_file && access(script_file, F_OK) == -1) {
        print_script_file_error();
        return EXIT_FAILURE;
    }

    // If there's a script file, execute commands from it
    if (script_file) {
        FILE *file = fopen(script_file, "r");
        if (!file) {
            print_script_file_error();
            return EXIT_FAILURE;
        }
        // char line[BUFFER_SIZE];
        // while (fgets(line, sizeof(line), file)) {
        //     print_command(line);
        //     add_to_history(line);
        //     char **args = parse_input(line);
        //     execute_command(args);
        //     free(args);
        // }
        char *buffer_h = NULL;
        size_t size_h = 0;
        ssize_t bytes_read_h;
        while (1) {
            bytes_read_h = getline(&buffer_h,&size_h, file);
            if (bytes_read_h == -1) break;
            if (bytes_read_h>0 && buffer_h[bytes_read_h-1] == '\n') {
                buffer_h[bytes_read_h-1] = '\0';
                vector_push_back(history, buffer_h);
            }
        }
        free(buffer_h);
        fclose(file);

        // char* line = NULL;
        // while (fgets(line, sizeof(line), file)) {
        //     print_command(line);
        //     add_to_history(line);
        //     char **args = parse_input(line);
        //     execute_command(args);
        //     free(args);
        // }
        // fclose(file);
    }

    // Start the interactive shell loop
    shell_loop();

    // Cleanup history and process list before exit
    vector_destroy(history);
    vector_destroy(process_list);

    return 0;
}

// The shell loop that runs interactively
void shell_loop() {
    char buffer[BUFFER_SIZE];
    char *input;
    char **args;

    // Setup signal handling for Ctrl+C
    signal(SIGINT, handle_sigint);

    while (1) {
        print_prompt(getcwd(NULL, 0), getpid());  // Print shell prompt

        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            if (feof(stdin)) {  // Handle EOF (Ctrl+D)
                break;
            }
            continue;
        }

        input = strdup(buffer);  // Duplicate buffer to preserve input
        if (input == NULL || strcmp(input, "\n") == 0) {
            free(input);
            continue;  // Ignore empty input
        }

        // add_to_history(input);  // Add command to history

        args = parse_input(input);  // Parse command into args
        if (args[0] == NULL) {
            free(args);
            free(input);
            continue;  // Ignore empty commands
        }

        // Handle built-in commands
        if (strcmp(args[0], "cd") == 0) {
            if (!shell_cd(args)) {
                print_no_directory(args[1]);
            }
        } else if (strcmp(args[0], "exit") == 0) {
            break;
        } else if (strcmp(args[0], "!history") == 0) {
            print_history_cmd();
        } else if (args[0][0] == '#') {
            size_t index = atoi(&args[0][1]);
            run_history_cmd(index);
        } else if (args[0][0] == '!') {
            run_prefix_cmd(args[0] + 1);
        } else {
            handle_logical_operators(args);  // Handle external commands and logical operators
        }

        if (args[0] != NULL && strcmp(args[0], "!history") != 0) {
            add_to_history(buffer);  // Add command to history if not !history
        }

        free(args);
        free(input);
    }

    save_history(history_file);  // Save history on exit
}

// Execute the given command
void execute_command(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        // In child process
        if (execvp(args[0], args) == -1) {
            print_exec_failed(args[0]);
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        // Fork failed
        print_fork_failed();
    } else {
        // In parent process
        print_command_executed(pid);
        add_process(args[0], pid);  // Track the process
        waitpid(pid, NULL, 0);  // Wait for child to complete
        remove_process(pid);  // Remove process after completion
    }
}

// Change directory (cd command)
int shell_cd(char **args) {
    if (args[1] == NULL) {
        print_invalid_command("cd");
        return 0;
    }
    if (chdir(args[1]) != 0) {
        return 0;  // Directory change failed
    }
    return 1;
}

// Add process to the list
void add_process(char *command, pid_t pid) {
    process *proc = malloc(sizeof(process));
    proc->command = strdup(command);
    proc->pid = pid;
    vector_push_back(process_list, proc);  // Add process to the list
}

// Remove process from the list
void remove_process(pid_t pid) {
    for (size_t i = 0; i < vector_size(process_list); i++) {
        process *proc = (process *)vector_get(process_list, i);
        if (proc->pid == pid) {
            free(proc->command);
            free(proc);
            vector_erase(process_list, i);  // Remove the process from the list
            break;
        }
    }
}

// Add command to history
void add_to_history(const char *command) {
    vector_push_back(history, (void *)command);  // Using vector push function
}

// Print the command history
void print_history_cmd() {
    for (size_t i = 0; i < vector_size(history); i++) {
        print_history_line(i, vector_get(history, i));  // Retrieve and print from vector
    }
}

// Run a command from history by index
void run_history_cmd(size_t index) {
    if (index >= vector_size(history)) {
        print_invalid_index();
        return;
    }
    char *cmd = vector_get(history, index);  // Get the command from history
    print_command(cmd);
    if (strstr(cmd, "cd ") != NULL) {
        char **args = parse_input(cmd);
        if (!shell_cd(args)) {
            print_no_directory(args[1]);
        }
        free(args);
        return;
    } else {
        char **args = parse_input(cmd);
        execute_command(args);
        free(args);
    }
}

// Run the last command with a given prefix
void run_prefix_cmd(const char *prefix) {
    for (ssize_t i = vector_size(history) - 1; i >= 0; i--) {
        if (strncmp(vector_get(history, i), prefix, strlen(prefix)) == 0) {
            print_command(vector_get(history, i));
            char **args = parse_input(vector_get(history, i));
            execute_command(args);
            free(args);
            return;
        }
    }
    print_no_history_match();
}

// Load history from a file
void load_history(const char *filename) {
    if (!filename) return;
    FILE *file = fopen(filename, "r");
    if (!file) {
        print_history_file_error();
        return;
    }
    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        vector_push_back(history, line);  // Push lines into history vector
    }
    fclose(file);
}

// Save history to a file
void save_history(const char *filename) {
    if (!filename) return;
    FILE *file = fopen(filename, "a");
    if (!file) {
        print_history_file_error();
        return;
    }
    for (size_t i = 0; i < vector_size(history); i++) {
        fprintf(file, "%s", (char *)vector_get(history, i));  // Save each command from the history
    }
    fclose(file);
}

// Parse input into arguments (tokenized)
char **parse_input(char *input) {
    int bufsize = BUFFER_SIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    if (!tokens) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(input, " \t\r\n\a");
    while (token != NULL) {
        tokens[position++] = token;

        if (position >= bufsize) {
            bufsize += BUFFER_SIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens) {
                fprintf(stderr, "Memory allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, " \t\r\n\a");
    }
    tokens[position] = NULL;
    return tokens;
}

// Handle logical operators
void handle_logical_operators(char **args) {
    int i = 0;
    int status;

    while (args[i] != NULL) {
        if (strcmp(args[i], "&&") == 0) {
            args[i] = NULL;  // Split the command at '&&'
            execute_command(args);  // Execute the first command

            waitpid(-1, &status, 0);  // Wait for the first command to complete

            // Only run the next command if the first succeeds (exit status == 0)
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                execute_command(&args[i + 1]);
            }
            return;
        } else if (strcmp(args[i], "||") == 0) {
            args[i] = NULL;  // Split the command at '||'
            execute_command(args);  // Execute the first command

            waitpid(-1, &status, 0);  // Wait for the first command to complete

            // Only run the next command if the first fails (exit status != 0)
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                execute_command(&args[i + 1]);
            }
            return;
        } else if (strcmp(args[i], ";") == 0) {
            args[i] = NULL;  // Split the command at ';'
            execute_command(args);  // Execute the first command

            waitpid(-1, &status, 0);  // Wait for the first command to complete

            // Always run the next command, regardless of the exit status
            execute_command(&args[i + 1]);
            return;
        }
        i++;
    }

    // If no logical operator, execute the command normally
    execute_command(args);
}
