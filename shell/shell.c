/**
 * shell
 * CS 341 - Fall 2024
 */

/*
I use Chatgpt to help me debug the operator part and read file part.
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
static char *history_path = NULL;

// Function declarations
void shell_loop();
void execute_command(char **args, char *input);
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
int external_command(char **args);
void wait_background_process();
void shell_ps();
int get_process_info(const char *proc_path, process_info *pinfo);
void handle_signal_command(char **args);

// void wait_background_process() {
//     int status;
//     pid_t pid;

//     // Reap all finished child processes
//     // while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
//     //     print_killed_process(pid, "Background process finished");  // Use appropriate print function
//     //     remove_process(pid);  // Clean up finished processes
//     // }
//     int length = vector_size(process_list);
//     for (int i = 0; i < length; i++) {
//         process *prcss = (process *) vector_get(process_list, i);
//         pid = waitpid(prcss->pid, &status, WNOHANG);
//         if (pid > 0) {
//             print_killed_process(pid, "Background process finished");
//             remove_process(pid);
//         }
//     }
//     // int status;
//     // pid_t pid;

//     // while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
//     // {
//     //     // Remove the process from process_list
//     //     remove_process(pid);

//     //     // Print a message indicating that the background process has completed
//     //     printf("Background process %d finished\n", pid);
//     //     fflush(stdout);
//     // }
// }

#include <time.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>

void wait_background_process() {
    int status;
    pid_t pid;

    // Iterate over all processes in the process list
    for (size_t i = 0; i < vector_size(process_list); i++) {
        process *prcss = (process *)vector_get(process_list, i);
        
        // Check if the background process has finished
        pid = waitpid(prcss->pid, &status, WNOHANG);
        
        if (pid > 0) {
            // Print a message indicating that the background process has completed
            print_killed_process(pid, prcss->command);  // Make sure this uses the command from process struct
            
            // Remove the process from the list
            remove_process(pid);
            i--;  // Adjust index due to removal
        }
    }
}




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
            printf("Unable to open script file!\n");
            print_script_file_error();
            return EXIT_FAILURE;
        }

        char *line = NULL; 
        size_t len = 0;
        ssize_t read; 
        while ((read = getline(&line, &len, file)) != -1) {
            print_command(line);
            add_to_history(line);
            char **args = parse_input(line);
            // printf("args[0]: %s\n", args[0]);
            // printf("args[1]: %s\n", args[1]);
            execute_command(args, line);
            free(args);
        }
        free(line);
        fclose(file);
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

        // Handle built-in commands kill, stop, or cont
        if (strcmp(args[0], "kill") == 0 || strcmp(args[0], "stop") == 0 || strcmp(args[0], "cont") == 0) {
            handle_signal_command(args);
        }


        // Handle built-in commands
        if (strcmp(args[0], "cd") == 0) {
            add_to_history(buffer);
            if (!shell_cd(args)) {
                print_no_directory(args[1]);
            }
        } else if (strcmp(args[0], "exit") == 0) {
            free(args);
            break;
        } else if (strcmp(args[0], "!history") == 0) {
            print_history_cmd();
        } else if (args[0][0] == '#') {
            size_t index = atoi(&args[0][1]);
            // printf("index: %ld\n", index);
            run_history_cmd(index);
        } else if (args[0][0] == '!') {
            run_prefix_cmd(args[0] + 1);
            // run_prefix_cmd(args);
        } else {
            // handle_logical_operators(args);  // Handle external commands and logical operators
            add_to_history(buffer);  // Add command to history if not !history
            execute_command(args, buffer);
            // add_to_history(buffer);  // Add command to history if not !history
        }

        // if (args[0] != NULL && strcmp(args[0], "!history") != 0) {
        //     add_to_history(buffer);  // Add command to history if not !history
        // }
        wait_background_process();
        free(args);
        free(input);
    }

    save_history(history_file);  // Save history on exit
}

// Execute the given command
void execute_command(char **args, char *input) {
    if (args[0] == NULL) {
        return;
    }

    // check if the command contains a logical operator
    if (strstr(input, "&&") != NULL || strstr(input, "||") != NULL || strstr(input, ";") != NULL) {
        // printf("args[0]: %s\n", args[0]);
        // printf("args[1]: %s\n", args[1]);
        // printf("args[2]: %s\n", args[2]);
        // printf("input: %s\n", input);
        // printf("success to execute logical\n");
        handle_logical_operators(args);
        return;
    }

    if (strcmp(args[0], "cd") == 0) {
        if (!shell_cd(args)) {
            print_no_directory(args[1]);
        }
        return;
    }

    if (strcmp(args[0], "exit") == 0) {
        exit(EXIT_SUCCESS);
    }

    if (strcmp(args[0], "!history") == 0) {
        print_history_cmd();
        return;
    }

    if (args[0][0] == '#') {
        size_t index = atoi(&args[0][1]);
        run_history_cmd(index);
        return;
    }

    if (args[0][0] == '!') {
        run_prefix_cmd(args[0] + 1);
        return;
    }

    // // check if the command contains a logical operator
    // if (strstr(input, "&&") != NULL || strstr(input, "||") != NULL || strstr(input, ";") != NULL) {
    //     printf("args[0]: %s\n", args[0]);
    //     printf("args[1]: %s\n", args[1]);
    //     printf("input: %s\n", input);
    //     printf("success to execute logical\n");
    //     handle_logical_operators(args);
    //     return;
    // }
    // printf("args[0]: %s\n", args[0]);
    // printf("args[1]: %s\n", args[1]);
    // printf("input: %s\n", input);
    // printf("input: %s\n", input+1);
    // printf("input: %s\n", input+2);
    // printf("fail to execute logical\n");
    //add_to_history(input);  
    // Implement the 'ps' command
    if (strcmp(args[0], "ps") == 0) {
        print_process_info_header();
        process_info pinfo;  // Declare a process_info struct
        for (size_t i = 0; i < vector_size(process_list); i++) {
            process *proc = (process *)vector_get(process_list, i);
            char proc_path[BUFFER_SIZE];
            snprintf(proc_path, sizeof(proc_path), "/proc/%d", proc->pid);

            if (get_process_info(proc_path, &pinfo)) {
                // Copy the command into pinfo's command field
                strncpy(pinfo.command, proc->command, sizeof(pinfo.command) - 1);
                pinfo.command[sizeof(pinfo.command) - 1] = '\0';
                print_process_info(&pinfo);
            }
        }
        return;
    }

    // Implement the signal commands: kill, stop, cont
    if (strcmp(args[0], "kill") == 0) {
        if (args[1] == NULL) {
            print_invalid_command("kill");
            return;
        }
        int target_pid = atoi(args[1]);
        if (kill(target_pid, SIGKILL) == 0) {
            print_killed_process(target_pid, "Killed");
        } else {
            print_no_process_found(target_pid);
        }
        return;
    }

    if (strcmp(args[0], "stop") == 0) {
        if (args[1] == NULL) {
            print_invalid_command("stop");
            return;
        }
        int target_pid = atoi(args[1]);
        if (kill(target_pid, SIGSTOP) == 0) {
            print_stopped_process(target_pid, "Stopped");
        } else {
            print_no_process_found(target_pid);
        }
        return;
    }

    if (strcmp(args[0], "cont") == 0) {
        if (args[1] == NULL) {
            print_invalid_command("cont");
            return;
        }
        int target_pid = atoi(args[1]);
        if (kill(target_pid, SIGCONT) == 0) {
            print_continued_process(target_pid, "Continued");
        } else {
            print_no_process_found(target_pid);
        }
        return;
    }

    // Check for redirection operators
    int fd;
    if (strstr(input, ">>") != NULL) {
        char *filename = strstr(input, ">>") + 2;
        while (*filename == ' ') filename++; // Skip spaces
        fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd < 0) {
            print_redirection_file_error();
            return;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
        input[strstr(input, ">>") - input] = '\0'; // Remove redirection part from input
    } else if (strstr(input, ">") != NULL) {
        char *filename = strstr(input, ">") + 1;
        while (*filename == ' ') filename++; // Skip spaces
        fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            print_redirection_file_error();
            return;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
        input[strstr(input, ">") - input] = '\0'; // Remove redirection part from input
    } else if (strstr(input, "<") != NULL) {
        char *filename = strstr(input, "<") + 1;
        while (*filename == ' ') filename++; // Skip spaces
        fd = open(filename, O_RDONLY);
        if (fd < 0) {
            print_redirection_file_error();
            return;
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
        input[strstr(input, "<") - input] = '\0'; // Remove redirection part from input
    }

    // Parse the cleaned-up command again to execute
    char **new_args = parse_input(input);
    external_command(new_args); // Execute the command
    free(new_args);
    // external_command(args);
}

// int external_command(char **args) {
//     int background = 0;
//     int len = 0;
//     while (args[len] != NULL) {
//         len++;
//     }
//     if (*args[len - 1] == '&') {
//         args[len - 1] = NULL;  // Remove '&' from the command
//         background = 1;
//     }

//     if(!strcmp(args[0], "cd")){
//         if (args[1] == NULL) {
//             print_no_directory("");
//             return 1;
//         }
//         // printf("is not cd\n");
//         if (chdir(args[1]) == -1) {
//             print_no_directory(args[1]);
//             return 1;
//         }
//         return 0;
//     }

//     if (args[0] == NULL) {
//         return 1;
//     }
//     fflush(stdout);
//     pid_t pid = fork();
//     if (pid == 0) {
//         // In child process
//         // check if the command is a background process
//         // printf("args[0]: %s\n", args[0]);
//         // printf("args[1]: %s\n", args[1]);
//         // int len = 0;
//         // while (args[len] != NULL) {
//         //     len++;
//         // }
//         // setpgid(0, 0);  // Set the process group ID in the child process
//         // if (*args[len - 1] == '&') {
//         //     args[len - 1] = NULL;  // Remove '&' from the command
//         // }
//         print_command_executed(getpid());
//         execvp(args[0], args);
//         print_exec_failed(args[0]);
//         exit(1);
//         // print_command_executed(getpid());
//         // execvp(args[0], args);
//         // print_exec_failed(args[0]);
//         // // printf("exit failure\n");
//         // exit(1);
//     }else if (pid < 0) {
//         print_fork_failed();
//         exit(1);
//     } else {
//         // In parent process
//         // check background process
//         int length = 0;
//         while (args[length] != NULL) {
//             length++;
//         }
//         printf("in background process\n");
//         printf("args[0]: %s\n", args[0]);
//         printf("args: %s\n", args[length - 1]);
//         if (background) {
//             // *args[length - 1] = '\0';  // Remove '&' from the command
//             // setpgid(0, 0);  // Set the process group ID to the process ID
//             // printf("check background process\n");
//             // add_process(args[0], pid);  // Add the process to the list
//             // return 0;
//             add_process(args[0], pid);  // Add the process to the list
//             return 0;
//         } else {
//             // Wait for the child process to complete
//             int status;
//             if (waitpid(pid, &status, 0) == -1) {
//                 print_wait_failed();
//                 exit(1);
//             }

//             if (WIFEXITED(status)) {
//                 return WEXITSTATUS(status);
//             }else{
//                 return 1;
//             }
//         }
//     }
//     return 1;
// }

// int external_command(char **args) {
//     int background = 0;
//     int len = 0;
    
//     // Determine the length of args
//     while (args[len] != NULL) {
//         len++;
//     }
    
//     // Check if the last argument is "&"
//     if (len > 0 && strcmp(args[len - 1], "&") == 0) {
//         args[len - 1] = NULL;  // Remove '&' from arguments
//         background = 1;
//     }
    
//     // Handle "cd" command
//     if (!strcmp(args[0], "cd")) {
//         if (args[1] == NULL) {
//             print_no_directory("");
//             return 1;
//         }
//         if (chdir(args[1]) == -1) {
//             print_no_directory(args[1]);
//             return 1;
//         }
//         return 0;
//     }
    
//     if (args[0] == NULL) {
//         return 1;
//     }
    
//     fflush(stdout);  // Flush output before forking
//     pid_t pid = fork();
    
//     if (pid == 0) {
//         // Child process
        
//         if (background) {
//             // Set a new process group for background processes
//             if (setpgid(0, 0) < 0) {
//                 perror("setpgid failed");
//                 exit(1);
//             }
//         }
        
//         print_command_executed(getpid());
//         execvp(args[0], args);
//         print_exec_failed(args[0]);
//         exit(1);
//     } else if (pid < 0) {
//         // Fork failed
//         print_fork_failed();
//         exit(1);
//     } else {
//         // Parent process
        
//         if (background) {
//             // Handle background process
//             printf("in background process\n");
//             printf("args[0]: %s\n", args[0]);
//             printf("args: &\n");  // Since '&' has been removed
//             add_process(args[0], pid);  // Add the process to the list
//             return 0;
//         } else {
//             // Handle foreground process
//             int status;
//             if (waitpid(pid, &status, 0) == -1) {
//                 print_wait_failed();
//                 exit(1);
//             }
    
//             if (WIFEXITED(status)) {
//                 return WEXITSTATUS(status);
//             } else {
//                 return 1;
//             }
//         }
//     }
    
//     return 1;
// }

// int external_command(char **args) {
//     int background = 0;
//     int len = 0;
//     while (args[len] != NULL) {
//         len++;
//     }
//     if (*args[len - 1] == '&') {
//         args[len - 1] = NULL;  // Remove '&' from the command
//         background = 1;
//     }

//     if (args[0] == NULL) {
//         return 1;
//     }

//     fflush(stdout);
//     pid_t pid = fork();
//     if (pid == 0) {
//         // In child process
//         // if (background) {
//         //     if (setpgid(0, 0) < 0) {
//         //         print_setpgid_failed();
//         //         exit(1);
//         //     }
//         // }

//         print_command_executed(getpid());  // Use provided print function
//         execvp(args[0], args);
//         print_exec_failed(args[0]);
//         exit(1);
//     } else if (pid < 0) {
//         print_fork_failed();
//         exit(1);
//     } else {
//         // In parent process
//         if (background) {
//             add_process(args[0], pid);  // Add the process to the list
//             return 0;
//         } else {
//             // Wait for the foreground process to complete
//             int status;
//             if (waitpid(pid, &status, 0) == -1) {
//                 print_wait_failed();
//                 exit(1);
//             }

//             if (WIFEXITED(status)) {
//                 return WEXITSTATUS(status);
//             } else {
//                 return 1;
//             }
//         }
//     }
//     return 1;
// }

int external_command(char **args) {
    int background = 0;
    int len = 0;

    // Determine the length of args
    while (args[len] != NULL) {
        len++;
    }

    // Check if the last argument is "&"
    if (len > 0 && strcmp(args[len - 1], "&") == 0) {
        args[len - 1] = NULL;  // Remove '&' from arguments
        background = 1;
    }

    if (args[0] == NULL) {
        return 1;
    }

    fflush(stdout);
    pid_t pid = fork();

    if (pid == 0) {
        // Child process
        if (background) {
            // Set a new process group for background processes
            if (setpgid(0, 0) < 0) {
                print_setpgid_failed();
                exit(1);
            }
        }

        print_command_executed(getpid());
        execvp(args[0], args);
        print_exec_failed(args[0]);
        exit(1);
    } else if (pid < 0) {
        // Fork failed
        print_fork_failed();
        exit(1);
    } else {
        // Parent process
        if (background) {
            add_process(args[0], pid);  // Add the process to the list
            printf("in background process\n"); // Debug: Make sure background process message is shown
            return 0;
        } else {
            // Wait for the foreground process to complete
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                print_wait_failed();
                exit(1);
            }

            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else {
                return 1;
            }
        }
    }
    return 1;
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
    add_to_history(vector_get(history, index));  // Add the command to history
    char *cmd = strdup(vector_get(history, index));  // Get the command from history
    print_command(cmd);
    // if (strstr(cmd, "cd ") != NULL) {
    //     char **args = parse_input(cmd);
    //     if (!shell_cd(args)) {
    //         print_no_directory(args[1]);
    //     }
    //     free(args);
    //     return;
    // } else {
    //     char **args = parse_input(cmd);
    //     // int i = 0;
    //     // while (args[i] != NULL) {
    //     //     printf("args[%d]: %s\n", i, args[i]);
    //     //     i++;
    //     // }
    //     // printf("cmd: %s\n", cmd);
    //     execute_command(args, vector_get(history, index));
    //     free(args);
    // }
    char **args = parse_input(cmd);
    execute_command(args, vector_get(history, index));
    free(args);
}

// Run the last command with a given prefix
void run_prefix_cmd(const char *prefix) {
    for (int i = vector_size(history) - 1; i >= 0; i--) {
        if (strncmp(vector_get(history, i), prefix, strlen(prefix)) == 0) {
            print_command(vector_get(history, i));
            char *input = strdup(vector_get(history, i));
            char **args = parse_input(input);
            // printf("Here's the history command: %s\n", (char *)vector_get(history, i));
            // puts(vector_get(history, i));
            add_to_history(vector_get(history, i));
            execute_command(args, vector_get(history, i));
            // add_to_history(vector_get(history, i));
            free(args);
            return;
        }
    }
    print_no_history_match();
}

// Load history from a file
void load_history(const char *filename) {
    char * fff = strdup(filename);
    if (!filename) return;
    history_path = get_full_path(fff);
    FILE *file = fopen(history_path, "r");
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
    FILE *file = fopen(history_path, "a");
    if (!file) {
        print_history_file_error();
        return;
    }
    for (size_t i = 0; i < vector_size(history); i++) {
        printf("%s", (char *)vector_get(history, i));
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
    int temp = 0;
    while (args[temp] != NULL) {
        temp++;
        // printf("temp[%d]: %s\n", temp, args[temp]);
    }
    // printf("temp: %d\n", temp);
    int i = 0;
    int status;

    while (args[i] != NULL) {
        if (!strcmp(args[i], "&&")) {
            // printf("1--------------------\n");
            //print args
            // for (int j = 0; j < i; j++) {
            //     printf("args[%d]: %s\n", j, args[j]);
            // }
            
            args[i] = NULL;  // Split the command at '&&'
            // execute_command(args);  // Execute the first command
            // com1  is command before && com2 is command after &&

            // printf("temp+1[%d]: %s\n", temp+ 1, args[temp+ 1]);
            char** com1 = malloc((i+1) * sizeof(char *)) ;
            for (int j = 0; j < i; j++) {
                com1[j] = strdup(args[j]);
                // printf("com1[%d]: %s\n", j, com1[j]);       
            }
            com1[i] = NULL;


            char **com2 = malloc((temp - i) * sizeof(char *));
            // copy the command after &&
            for (int j = i + 1; j < temp; j++) {
                com2[j - i - 1] = strdup(args[j]);
                // printf("com2[%d]: %s\n", j - i - 1, com2[j - i - 1]);
            }
            com2[temp - i - 1] = NULL;
            // print com1

            // print com2
            // for(int j = 0; j < temp - i - 1; j++) {
            //     printf("com2[%d]: %s\n", j, com2[j]);
            // }
            //printf("*");
            int a = external_command(com1);
            // printf("a: %d\n", a);
            if(!a) {
                // printf("*");
                external_command(com2);
            }


            waitpid(-1, &status, 0);  // Wait for the first command to complete


            return;
        } else if (strcmp(args[i], "||") == 0) {
            // printf("2--------------------\n");
            args[i] = NULL;  // Split the command at '&&'
            // execute_command(args);  // Execute the first command
            // com1  is command before && com2 is command after &&

            // printf("temp+1[%d]: %s\n", temp+ 1, args[temp+ 1]);
            char** com1 = malloc((i+1) * sizeof(char *)) ;
            for (int j = 0; j < i; j++) {
                com1[j] = strdup(args[j]);
                //printf("com1[%d]: %s\n", j, com1[j]);       
            }
            com1[i] = NULL;


            char **com2 = malloc((temp - i) * sizeof(char *));
            // copy the command after &&
            for (int j = i + 1; j < temp; j++) {
                com2[j - i - 1] = strdup(args[j]);
                //printf("com2[%d]: %s\n", j - i - 1, com2[j - i - 1]);
            }
            com2[temp - i - 1] = NULL;
            // print com1

            // print com2
            // for(int j = 0; j < temp - i - 1; j++) {
            //     printf("com2[%d]: %s\n", j, com2[j]);
            // }
            //printf("*");
            int a = external_command(com1);
            // printf("a: %d\n", a);
            if(a) {
                // printf("*");
                external_command(com2);
            }


            waitpid(-1, &status, 0);  // Wait for the first command to complete

            // // Only run the next command if the first succeeds (exit status == 0)
            // if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            //     // execute_command(&args[i + 1]);
            //     external_command(&args[i + 1]);
            // }
            return;
        } else if (args[i][strlen(args[i]) - 1] == ';') {
            // printf("3--------------------\n");
            char** com1 = malloc((i+2) * sizeof(char *)) ;
            for (int j = 0; j <= i; j++) {
                com1[j] = strdup(args[j]);
                // printf("com1[%d]: %s\n", j, com1[j]);       
            }
            com1[i][strlen(args[i]) - 1] = '\0';
            com1[i+1] = NULL;


            char **com2 = malloc((temp - i) * sizeof(char *));
            // copy the command after &&
            for (int j = i + 1; j < temp; j++) {
                com2[j - i - 1] = strdup(args[j]);
                // printf("com2[%d]: %s\n", j - i - 1, com2[j - i - 1]);
            }
            com2[temp - i - 1] = NULL;
            // print com1

            // print com2
            // for(int j = 0; j < temp - i - 1; j++) {
            //     printf("com2[%d]: %s\n", j, com2[j]);
            // }
            //printf("*");
            external_command(com1);
            // printf("*");
            external_command(com2);


            waitpid(-1, &status, 0);  // Wait for the first command to complete
            return;
        }
        i++;
    }

    // If no logical operator, execute the command normally
    // execute_command(args);
    external_command(args);
}

void shell_ps() {
    print_process_info_header();

    // Include the shell process itself
    process_info pinfo;
    char proc_path[BUFFER_SIZE];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d", getpid());
    if (get_process_info(proc_path, &pinfo)) {
        print_process_info(&pinfo);
    }

    // Now print information about all active child processes
    for (size_t i = 0; i < vector_size(process_list); i++) {
        process *proc = (process *)vector_get(process_list, i);
        snprintf(proc_path, sizeof(proc_path), "/proc/%d", proc->pid);
        if (get_process_info(proc_path, &pinfo)) {
            pinfo.command = proc->command;
            print_process_info(&pinfo);
        }
    }
}

int get_process_info(const char *proc_path, process_info *pinfo) {
    char stat_path[BUFFER_SIZE], status_path[BUFFER_SIZE];
    FILE *stat_file, *status_file;
    char buffer[BUFFER_SIZE];
    
    // Open /proc/[pid]/stat file to read process statistics
    snprintf(stat_path, sizeof(stat_path), "%s/stat", proc_path);
    stat_file = fopen(stat_path, "r");
    if (!stat_file) return 0;

    // Read the relevant information from /proc/[pid]/stat
    unsigned long utime, stime, starttime;
    // unsigned long long start_time_ticks;
    fscanf(stat_file, "%d %*s %c %*d %*d %*d %*d %*d %lu %lu %*lu %*lu %*lu %*lu %*lu %*lu %lu %*ld %*ld %*ld %*ld %lu",
           &pinfo->pid, &pinfo->state, &utime, &stime, &pinfo->vsize, &starttime);
    fclose(stat_file);

    // Open /proc/[pid]/status file to read the number of threads
    snprintf(status_path, sizeof(status_path), "%s/status", proc_path);
    status_file = fopen(status_path, "r");
    if (!status_file) return 0;
    
    while (fgets(buffer, BUFFER_SIZE, status_file)) {
        if (strncmp(buffer, "Threads:", 8) == 0) {
            sscanf(buffer, "Threads: %ld", &pinfo->nthreads);
            break;
        }
    }
    fclose(status_file);

    // Get system uptime to calculate the start time of the process
    struct sysinfo sys_info;
    if (sysinfo(&sys_info) != 0) return 0;
    time_t boot_time = time(NULL) - sys_info.uptime; // System boot time

    // Calculate the process start time
    time_t process_start_time = boot_time + (starttime / sysconf(_SC_CLK_TCK));
    struct tm *start_tm = localtime(&process_start_time);
    strftime(pinfo->start_str, sizeof(pinfo->start_str), "%H:%M", start_tm);

    // Calculate total CPU time (utime + stime) in seconds
    unsigned long total_time = (utime + stime) / sysconf(_SC_CLK_TCK);
    snprintf(pinfo->time_str, sizeof(pinfo->time_str), "%lu:%02lu", total_time / 60, total_time % 60);

    return 1;
}


void handle_signal_command(char **args) {
    if (args[1] == NULL) {
        if (strcmp(args[0], "kill") == 0) {
            print_invalid_command("kill");
        } else if (strcmp(args[0], "stop") == 0) {
            print_invalid_command("stop");
        } else if (strcmp(args[0], "cont") == 0) {
            print_invalid_command("cont");
        }
        return;
    }

    pid_t pid = atoi(args[1]);
    if (strcmp(args[0], "kill") == 0) {
        if (kill(pid, SIGKILL) == 0) {
            print_killed_process(pid, "killed");
        } else {
            print_no_process_found(pid);
        }
    } else if (strcmp(args[0], "stop") == 0) {
        if (kill(pid, SIGSTOP) == 0) {
            print_stopped_process(pid, "stopped");
        } else {
            print_no_process_found(pid);
        }
    } else if (strcmp(args[0], "cont") == 0) {
        if (kill(pid, SIGCONT) == 0) {
            print_continued_process(pid, "continued");
        } else {
            print_no_process_found(pid);
        }
    }
}

