// /**
//  * deepfried_dd
//  * CS 341 - Fall 2024
//  */
// #include "format.h"
// #include "signal.h"
// #include "time.h"
// #include <linux/time.h>

// static struct time start, end;
// static int ctrl_d = 0; 

// static char *inputFile = NULL;   
// static char *outputFile = NULL;  
// static size_t blockSize = 0;
// static size_t count = 0;
// static size_t skipInput = 0;
// static size_t skipOutput =  0;

// void signal_handler() {
//     ctrl_d = 1;
// }

// void parse_arg(int argc, char **argv) {
//     int opt;
//     while ((opt = getopt(argc, argv, "i:o:b:c:p:k:")) != -1) {
//         switch (opt) {
//             case 'i':
//                 inputFile = fopen(optarg, "r");
//                 if (!inputFile) {
//                     print_invalid_input(optarg);
//                     exit(1);
//                 }
//                 break;
//             case 'o':
//                 outputFile = fopen(optarg, "w+");
//                 if (!outputFile) {
//                     print_invalid_output(optarg);
//                     exit(1);
//                 }
//                 break;
//             case 'b':
//                 blockSize = atoi(optarg);
//                 break;
//             case 'c':
//                 count = atoi(optarg);
//                 break;
//             case 'p':
//                 skipInput = atoi(optarg);
//                 break;
//             case 'k':
//                 skipOutput = atoi(optarg);
//                 break;
//             default:
//                 exit(1);
//         }
//     }
// }

// int main(int argc, char **argv) {
//     clock_gettime(CLOCK_REALTIME, &start);
//     signal(SIGUSR1, signal_handler);
//     parse_arg(argc, argv);
//     return 0;
// }


/**
 * deepfried_dd
 * CS 341 - Fall 2024
 */
#include "format.h"
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static struct timespec start, end;
static int ctrl_d = 0;

// Argument variables
static FILE *inputFile = NULL;   
static FILE *outputFile = NULL;  
static size_t blockSize = 512;  // Default block size
static size_t count = 0;        // Default is to copy the entire file
static size_t skipInput = 0;
static size_t skipOutput = 0;

// Status tracking
static size_t full_blocks_in = 0;
static size_t partial_blocks_in = 0;
static size_t full_blocks_out = 0;
static size_t partial_blocks_out = 0;
static size_t total_bytes_copied = 0;

// Signal handler for SIGUSR1
void signal_handler() {
    ctrl_d = 1;  // Set flag to print report elsewhere
}

// Argument parsing function
void parse_arg(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "i:o:b:c:p:k:")) != -1) {
        switch (opt) {
            case 'i':
                inputFile = fopen(optarg, "rb");
                if (!inputFile) {
                    print_invalid_input(optarg);
                    exit(1);
                }
                break;
            case 'o':
                outputFile = fopen(optarg, "wb+");
                if (!outputFile) {
                    print_invalid_output(optarg);
                    exit(1);
                }
                break;
            case 'b':
                blockSize = atoi(optarg);
                if (blockSize <= 0) {
                    fprintf(stderr, "Invalid block size\n");
                    exit(1);
                }
                break;
            case 'c':
                count = atoi(optarg);
                break;
            case 'p':
                skipInput = atoi(optarg);
                break;
            case 'k':
                skipOutput = atoi(optarg);
                break;
            default:
                exit(1);
        }
    }

    // Default to stdin and stdout if not specified
    if (!inputFile) inputFile = stdin;
    if (!outputFile) outputFile = stdout;
}

// Function to print status report using helper function
void print_dd_status_report() {
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    print_status_report(full_blocks_in, partial_blocks_in, full_blocks_out, 
                        partial_blocks_out, total_bytes_copied, time_elapsed);
}

int main(int argc, char **argv) {
    // Start timing and set up signal handler
    clock_gettime(CLOCK_REALTIME, &start);
    signal(SIGUSR1, signal_handler);

    // Parse command-line arguments
    parse_arg(argc, argv);

    // Skip blocks in input file
    if (skipInput > 0 && inputFile != stdin) {
        if (fseek(inputFile, skipInput * blockSize, SEEK_SET) != 0) {
            perror("Failed to skip blocks in input file");
            fclose(inputFile);
            fclose(outputFile);
            exit(1);
        }
    }

    // Skip blocks in output file
    if (skipOutput > 0 && outputFile != stdout) {
        if (fseek(outputFile, skipOutput * blockSize, SEEK_SET) != 0) {
            perror("Failed to skip blocks in output file");
            fclose(inputFile);
            fclose(outputFile);
            exit(1);
        }
    }

    // Allocate buffer for block copying
    char *buffer = malloc(blockSize);
    if (!buffer) {
        perror("Failed to allocate memory for buffer");
        fclose(inputFile);
        fclose(outputFile);
        exit(1);
    }

    // Main copy loop
    size_t blocksCopied = 0;
    while ((count == 0 || blocksCopied < count) && !feof(inputFile)) {
        // Read a block from input
        size_t bytesRead = fread(buffer, 1, blockSize, inputFile);
        if (bytesRead == 0) break;  // EOF or read error

        if (bytesRead == blockSize) {
            full_blocks_in++;
        } else {
            partial_blocks_in++;
        }

        // Write the block to output
        size_t bytesWritten = fwrite(buffer, 1, bytesRead, outputFile);
        if (bytesWritten < bytesRead) {
            perror("Failed to write block to output file");
            free(buffer);
            fclose(inputFile);
            fclose(outputFile);
            exit(1);
        }

        if (bytesWritten == blockSize) {
            full_blocks_out++;
        } else {
            partial_blocks_out++;
        }

        total_bytes_copied += bytesRead;
        blocksCopied++;

        // Check if SIGUSR1 was received for live status report
        if (ctrl_d) {
            print_dd_status_report();
            ctrl_d = 0;
        }
    }

    // Final status report
    print_dd_status_report();

    // Clean up
    free(buffer);
    if (inputFile != stdin) fclose(inputFile);
    if (outputFile != stdout) fclose(outputFile);

    return 0;
}
