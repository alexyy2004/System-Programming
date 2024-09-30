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
#include <errno.h>


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
void handle_logical_operators(char **args, char *input);
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
int external_command(char **args, char *input);
void wait_background_process();
void handle_signal_commands(char **args);
void send_signal_to_process(pid_t pid, int signal, const char *command);
int is_child_process(pid_t pid);
process* get_process(pid_t pid);
void handle_ps_command();
process_info* create_process_info(process *proc); //todo
void destory_process_info(process_info* info); //todo

void handle_ps_command() {
    print_process_info_header();
    size_t proc_length = vector_size(process_list);
    printf("proc_length: %ld\n", proc_length);
    for (size_t i = 0; i < proc_length; i++) {
        process *proc = (process *)vector_get(process_list, i);
        process_info *info = create_process_info(proc);
        print_process_info(info);
        destory_process_info(info);
    }
    // create a new process shell
    process *shell = malloc(sizeof(process));
    shell->command = "./shell";
    shell->pid = getpid();
    process_info *shell_info = create_process_info(shell);
    print_process_info(shell_info);
    destory_process_info(shell_info);
}

// process_info* create_process_info(process *proc) {
//     process_info *info = malloc(sizeof(process_info));
//     if (!info) {
//         // Handle allocation failure
//         return NULL;
//     }

//     // Initialize fields
//     info->pid = proc->pid;
//     info->command = strdup(proc->command); // Make a copy of the command

//     // Paths to /proc files
//     char stat_path[256];
//     snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", proc->pid);

//     FILE *stat_file = fopen(stat_path, "r");
//     if (!stat_file) {
//         // Process might have terminated
//         info->nthreads = 0;
//         info->vsize = 0;
//         info->state = 'Z'; // Zombie or unknown
//         info->start_str = strdup("??:??");
//         info->time_str = strdup("??:??");
//         return info;
//     }

//     // Read the entire stat line
//     char stat_line[1024];
//     if (fgets(stat_line, sizeof(stat_line), stat_file) == NULL) {
//         fclose(stat_file);
//         info->nthreads = 0;
//         info->vsize = 0;
//         info->state = 'Z'; // Zombie or unknown
//         info->start_str = strdup("??:??");
//         info->time_str = strdup("??:??");
//         return info;
//     }
//     fclose(stat_file);

//     // Parse the stat_line (similar to previous implementation)
//     // ...

//     // After parsing, set the fields appropriately
//     info->nthreads = num_threads;
//     info->vsize = vsize; // in kilobytes
//     info->state = state_char;

//     // Allocate and set start_str and time_str
//     info->start_str = strdup(start_time_str);
//     info->time_str = strdup(cpu_time_str);

//     return info;
// }

process_info* create_process_info(process *proc) {
    process_info *info = malloc(sizeof(process_info));
    if (!info) {
        // Handle allocation failure
        return NULL;
    }

    info->pid = proc->pid;
    info->command = strdup(proc->command); // Make a copy of the command

    // Paths to /proc files
    char stat_path[256];
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", proc->pid);

    FILE *stat_file = fopen(stat_path, "r");
    if (!stat_file) {
        // Process might have terminated or permission denied
        info->nthreads = 0;
        info->vsize = 0;
        info->state = 'Z'; // Zombie or unknown
        info->start_str = strdup("??:??");
        info->time_str = strdup("??:??");
        return info;
    }

    // Read the entire stat line
    char stat_line[1024];
    if (fgets(stat_line, sizeof(stat_line), stat_file) == NULL) {
        fclose(stat_file);
        info->nthreads = 0;
        info->vsize = 0;
        info->state = 'Z'; // Zombie or unknown
        info->start_str = strdup("??:??");
        info->time_str = strdup("??:??");
        return info;
    }
    fclose(stat_file);

    // Parse the stat_line
    // The comm field might contain spaces and is enclosed in parentheses.
    // We need to find the closing parenthesis.
    char *close_paren = strrchr(stat_line, ')');
    if (!close_paren) {
        // Malformed stat file
        info->nthreads = 0;
        info->vsize = 0;
        info->state = 'Z';
        info->start_str = strdup("??:??");
        info->time_str = strdup("??:??");
        return info;
    }

    // Extract pid and comm
    int stat_pid;
    char comm[256];
    sscanf(stat_line, "%d (%[^)])", &stat_pid, comm);

    // Get the rest of the fields
    char *rest = close_paren + 2; // Skip ") "

    // Split rest into tokens
    #define MAX_STAT_FIELDS 64
    char *stat_fields[MAX_STAT_FIELDS];
    int field_count = 0;

    char *field = strtok(rest, " ");
    while (field != NULL && field_count < MAX_STAT_FIELDS) {
        stat_fields[field_count++] = field;
        field = strtok(NULL, " ");
    }

    if (field_count < 22) {
        // Not enough fields
        info->nthreads = 0;
        info->vsize = 0;
        info->state = 'Z';
        info->start_str = strdup("??:??");
        info->time_str = strdup("??:??");
        return info;
    }

    // Extract necessary fields
    char state_char = stat_fields[0][0];
    unsigned long utime = strtoul(stat_fields[13], NULL, 10);
    unsigned long stime = strtoul(stat_fields[14], NULL, 10);
    int num_threads = atoi(stat_fields[17]);
    unsigned long long starttime = strtoull(stat_fields[19], NULL, 10);
    unsigned long vsize = strtoul(stat_fields[21], NULL, 10) / 1024; // Convert to kilobytes

    // Read btime from /proc/stat
    FILE *proc_stat_file = fopen("/proc/stat", "r");
    if (!proc_stat_file) {
        // Cannot read /proc/stat
        info->nthreads = num_threads;
        info->vsize = vsize;
        info->state = state_char;
        info->start_str = strdup("??:??");
        info->time_str = strdup("??:??");
        return info;
    }

    char proc_stat_line[256];
    unsigned long btime = 0;
    while (fgets(proc_stat_line, sizeof(proc_stat_line), proc_stat_file)) {
        if (strncmp(proc_stat_line, "btime ", 6) == 0) {
            btime = strtoul(proc_stat_line + 6, NULL, 10);
            break;
        }
    }
    fclose(proc_stat_file);

    if (btime == 0) {
        // Could not find btime
        info->nthreads = num_threads;
        info->vsize = vsize;
        info->state = state_char;
        info->start_str = strdup("??:??");
        info->time_str = strdup("??:??");
        return info;
    }

    // Calculate the process start time
    unsigned long hertz = sysconf(_SC_CLK_TCK);
    unsigned long long total_start_time = btime + (starttime / hertz);
    time_t proc_start_time = (time_t)total_start_time;
    struct tm *start_tm = localtime(&proc_start_time);
    if (!start_tm) {
        info->start_str = strdup("??:??");
    } else {
        char start_time_str[16];
        snprintf(start_time_str, sizeof(start_time_str), "%02d:%02d",
                 start_tm->tm_hour, start_tm->tm_min);
        info->start_str = strdup(start_time_str);
    }

    // Calculate CPU time
    unsigned long total_time_in_sec = (utime + stime) / hertz;
    unsigned long minutes = total_time_in_sec / 60;
    unsigned long seconds = total_time_in_sec % 60;
    char cpu_time_str[16];
    snprintf(cpu_time_str, sizeof(cpu_time_str), "%lu:%02lu", minutes, seconds);
    info->time_str = strdup(cpu_time_str);

    // Set remaining fields
    info->nthreads = num_threads;
    info->vsize = vsize;
    info->state = state_char;

    return info;
}


void destory_process_info(process_info* info) {
    if (info) {
        free(info->start_str);
        free(info->time_str);
        free(info->command);
        free(info);
    }
}

process* get_process(pid_t pid) {
    for (size_t i = 0; i < vector_size(process_list); i++) {
        process *proc = (process *)vector_get(process_list, i);
        if (proc->pid == pid) {
            return proc; // Found the process
        }
    }
    return NULL; // Not found
}

void wait_background_process() {
    int status;
    pid_t pid;

    // Reap all finished child processes
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("Background process finished, pid: %d\n", pid);
        remove_process(pid);  // Clean up finished processes
    }
    return;
}

void handle_signal_commands(char **args) {
    if (strcmp(args[0], "kill") == 0) {
        // Handle kill command
        if (args[1] == NULL) {
            print_invalid_command("kill");
            return;
        }
        pid_t pid = atoi(args[1]);
        send_signal_to_process(pid, SIGKILL, "kill");
    } else if (strcmp(args[0], "stop") == 0) {
        // Handle stop command
        if (args[1] == NULL) {
            print_invalid_command("stop");
            return;
        }
        pid_t pid = atoi(args[1]);
        send_signal_to_process(pid, SIGSTOP, "stop");
    } else if (strcmp(args[0], "cont") == 0) {
        // Handle cont command
        if (args[1] == NULL) {
            print_invalid_command("cont");
            return;
        }
        pid_t pid = atoi(args[1]);
        send_signal_to_process(pid, SIGCONT, "cont");
    }
}

void send_signal_to_process(pid_t pid, int signal, const char *command) {
    // Check if pid is a child process
    if (!is_child_process(pid)) {
        if (strcmp(command, "kill") == 0) {
            print_no_process_found(pid);
        } else if (strcmp(command, "stop") == 0) {
            print_no_process_found(pid);
        } else if (strcmp(command, "cont") == 0) {
            print_no_process_found(pid);
        }
        return;
    }

    // Send the signal
    if (kill(pid, signal) == -1) {
        // Handle error
        if (errno == ESRCH) {
            // No such process
            if (strcmp(command, "kill") == 0) {
                print_no_process_found(pid);
            } else if (strcmp(command, "stop") == 0) {
                print_no_process_found(pid);
            } else if (strcmp(command, "cont") == 0) {
                print_no_process_found(pid);
            }
        } else {
            // Other error
            perror("kill");
        }
    } else {
        // Success
        process *proc = get_process(pid); // You need to implement this function to retrieve the process struct
        if (strcmp(command, "kill") == 0) {
            if (proc != NULL) {
                print_killed_process(pid, proc->command);
                remove_process(pid);
            }
        } else if (strcmp(command, "stop") == 0) {
            if (proc != NULL) {
                print_stopped_process(pid, proc->command);
                // remove_process(pid);
            }
        } else if (strcmp(command, "cont") == 0) {
            if (proc != NULL) {
                print_continued_process(pid, proc->command);
                // remove_process(pid);
            }
        }
    }
}

int is_child_process(pid_t pid) {
    for (size_t i = 0; i < vector_size(process_list); i++) {
        process *proc = (process *)vector_get(process_list, i);
        if (proc->pid == pid) {
            return 1; // Found the process
        }
    }
    return 0; // Not a child process
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
    signal(SIGCHLD, wait_background_process);

    while (1) {
        char *cwd = getcwd(NULL, 0);
        print_prompt(cwd, getpid());  // Print shell prompt
        free(cwd);

        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            if (feof(stdin)) {  // Handle EOF (Ctrl+D)
                break;
            }
            continue;
        }

        buffer[strcspn(buffer, "\n")] = '\0';  // Remove trailing newline
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

    // Check for signal commands
    if (strcmp(args[0], "kill") == 0 || strcmp(args[0], "stop") == 0 || strcmp(args[0], "cont") == 0) {
        handle_signal_commands(args);
        return;
    }
    
    // Check for ps command
    if (strcmp(args[0], "ps") == 0) {
        handle_ps_command();
        return;
    }

    // check if the command contains a logical operator
    if (strstr(input, "&&") != NULL || strstr(input, "||") != NULL || strstr(input, ";") != NULL) {
        // printf("args[0]: %s\n", args[0]);
        // printf("args[1]: %s\n", args[1]);
        // printf("args[2]: %s\n", args[2]);
        // printf("input: %s\n", input);
        // printf("success to execute logical\n");
        handle_logical_operators(args, input);
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
    external_command(args, input);
}


int external_command(char **args, char *input) {
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
    
    // Redirection handling
    int fd; // File descriptor
    int redirect_output = 0, redirect_append = 0, redirect_input = 0;
    char *filename = NULL;

    // Check for output redirection ">"
    for (int i = 0; i < len; i++) {
        if (args[i] != NULL && strcmp(args[i], ">") == 0) {
            if (args[i + 1] != NULL) {
                filename = args[i + 1];
                redirect_output = 1;
                args[i] = NULL;  // Truncate the command here
                break;
            } else {
                print_redirection_file_error(); // Handle the case where there's no filename
                return 1;
            }
        }
    }

    // Check for append redirection ">>"
    for (int i = 0; i < len; i++) {
        if (args[i] != NULL && strcmp(args[i], ">>") == 0) {
            if (args[i + 1] != NULL) {
                filename = args[i + 1];
                redirect_append = 1;
                args[i] = NULL;  // Truncate the command here
                break;
            } else {
                print_redirection_file_error(); // Handle the case where there's no filename
                return 1;
            }
        }
    }

    // Check for input redirection "<"
    for (int i = 0; i < len; i++) {
        if (args[i] != NULL && strcmp(args[i], "<") == 0) {
            if (args[i + 1] != NULL) {
                filename = args[i + 1];
                redirect_input = 1;
                args[i] = NULL;  // Truncate the command here
                break;
            } else {
                print_redirection_file_error(); // Handle the case where there's no filename
                return 1;
            }
        }
    }
    
    if (args[0] == NULL) {
        return 1;
    }
    
    fflush(stdin);
    fflush(stdout);  // Flush output before forking
    pid_t pid = fork();
    
    if (pid == 0) { // Child process
        // Handle output redirection
        if (redirect_output) {
            fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                print_redirection_file_error();
                exit(1);
            }
            if (dup2(fd, STDOUT_FILENO) < 0) {  // Redirect stdout to the file
                print_redirection_file_error();
                close(fd);
                exit(1);
            }
            close(fd);
        }
        
        // Handle append redirection
        if (redirect_append) {
            fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) {
                print_redirection_file_error();
                exit(1);
            }
            if (dup2(fd, STDOUT_FILENO) < 0) {  // Redirect stdout to the file
                print_redirection_file_error();
                close(fd);
                exit(1);
            }
            close(fd);
        }
        
        // Handle input redirection
        if (redirect_input) {
            fd = open(filename, O_RDONLY);
            if (fd < 0) {
                print_redirection_file_error();
                exit(1);
            }
            if (dup2(fd, STDIN_FILENO) < 0) {  // Redirect stdin to the file
                print_redirection_file_error();
                close(fd);
                exit(1);
            }
            close(fd);
        }
        
        // Execute the command
        print_command_executed(getpid());  // Ensure this is formatted correctly
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
            add_process(input, pid);  // Add the process to the list
            return 0;
        } else {
            // Handle foreground process
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
void handle_logical_operators(char **args, char *input) {
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
            int a = external_command(com1, input);
            // printf("a: %d\n", a);
            if(!a) {
                // printf("*");
                external_command(com2, input);
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
            int a = external_command(com1, input);
            // printf("a: %d\n", a);
            if(a) {
                // printf("*");
                external_command(com2, input);
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
            external_command(com1, input);
            // printf("*");
            external_command(com2, input);


            waitpid(-1, &status, 0);  // Wait for the first command to complete
            
            // free the memory of com1 com2
            for (int j = 0; com1[j] != NULL; j++) {
                free(com1[j]);
            }
            free(com1);

            for (int j = 0; com2[j] != NULL; j++) {
                free(com2[j]);
            }
            free(com2);
            
            return;
        }
        i++;
    }

    // If no logical operator, execute the command normally
    // execute_command(args);
    external_command(args, input);
}