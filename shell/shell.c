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

// Global variables for history
static vector *history;
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

// Signal handler for SIGINT (Ctrl+C)
void handle_sigint(int sig) {
    (void)sig;  // Avoid unused parameter warning
    printf("\n");
    print_prompt(getcwd(NULL, 0), getpid());  // Reprint prompt after signal
    fflush(stdout);
}

// Main entry point for the shell
int shell(int argc, char *argv[]) {
    int opt;
    char *script_file = NULL;

    // Initialize history vector
    history = string_vector_create();

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
        char line[BUFFER_SIZE];
        while (fgets(line, sizeof(line), file)) {
            print_command(line);
            add_to_history(line);
            char **args = parse_input(line);
            execute_command(args);
            free(args);
        }
        fclose(file);
    }

    // Start the interactive shell loop
    shell_loop();

    // Cleanup history before exit
    vector_destroy(history);

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

        add_to_history(input);  // Add command to history

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
        waitpid(pid, NULL, 0);  // Wait for child to complete
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
    char **args = parse_input(cmd);
    execute_command(args);
    free(args);
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

// Handle logical operators (&&, ||, ;)
void handle_logical_operators(char **args) {
    int i = 0;
    while (args[i] != NULL) {
        puts(args[i]);
        if (strcmp(args[i], "&&") == 0) {
            args[i] = NULL;
            execute_command(args);  // Execute first command
            int status;
            waitpid(-1, &status, 0);
            if (WEXITSTATUS(status) == 0) {  // Only run next if first succeeds
                execute_command(&args[i + 1]);
            }
            return;
        } else if (strcmp(args[i], "||") == 0) {
            args[i] = NULL;
            execute_command(args);  // Execute first command
            int status;
            waitpid(-1, &status, 0);
            if (WEXITSTATUS(status) != 0) {  // Only run next if first fails
                execute_command(&args[i + 1]);
            }
            return;
        } else if (strcmp(args[i], ";") == 0) {
            args[i] = NULL;
            execute_command(args);  // Execute first command
            execute_command(&args[i + 1]);  // Always execute next
            return;
        }
        i++;
    }
    execute_command(args);  // No operator, execute normally
}
