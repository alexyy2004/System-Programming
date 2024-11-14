/**
 * deepfried_dd
 * CS 341 - Fall 2024
 * [Group Working]
 * Group Member Netids: pjame2, boyangl3, yueyan2, hengl2, liu327
 */
#include "format.h"
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static struct timespec start, end;
static int ctrl_d = 0;

static FILE *input_file = NULL;   
static FILE *output_file = NULL;  
static size_t block_size = 512;
static size_t count = 0;
static size_t skip_input = 0;
static size_t skip_output = 0;

static size_t full_blocks_in = 0;
static size_t partial_blocks_in = 0;
static size_t full_blocks_out = 0;
static size_t partial_blocks_out = 0;
static size_t total_bytes_copied = 0;

void signal_handler() {
    ctrl_d = 1; 
}

void parse_arg(int argc, char **argv) { // written by chatgpt
    int opt;
    while ((opt = getopt(argc, argv, "i:o:b:c:p:k:")) != -1) {
        switch (opt) {
            case 'i':
                input_file = fopen(optarg, "rb");
                if (!input_file) {
                    print_invalid_input(optarg);
                    exit(1);
                }
                break;
            case 'o':
                output_file = fopen(optarg, "wb+");
                if (!output_file) {
                    print_invalid_output(optarg);
                    exit(1);
                }
                break;
            case 'b':
                block_size = atoi(optarg);
                if (block_size <= 0) {
                    fprintf(stderr, "Invalid block size\n");
                    exit(1);
                }
                break;
            case 'c':
                count = atoi(optarg);
                break;
            case 'p':
                skip_input = atoi(optarg);
                break;
            case 'k':
                skip_output = atoi(optarg);
                break;
            default:
                exit(1);
        }
    }

    // Default to stdin and stdout if not specified
    if (!input_file) input_file = stdin;
    if (!output_file) output_file = stdout;
}

void print_dd_status_report() {
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_elapsed = difftime(end.tv_sec, start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    print_status_report(full_blocks_in, partial_blocks_in, full_blocks_out, partial_blocks_out, total_bytes_copied, time_elapsed);
}

void my_dd() {
    if (skip_input > 0 && input_file != stdin) {
        if (fseek(input_file, skip_input * block_size, SEEK_SET) != 0) {
            perror("Fail to skip blocks in input file\n");
            fclose(input_file);
            fclose(output_file);
            exit(1);
        }
    }

    if (skip_output > 0 && output_file != stdout) {
        if (fseek(output_file, skip_output * block_size, SEEK_SET) != 0) {
            perror("Fail to skip blocks in output file\n");
            fclose(input_file);
            fclose(output_file);
            exit(1);
        }
    }

    char *buffer = malloc(block_size);
    memset(buffer, 0, block_size);

    size_t blocks_copied = 0;
    while ((count == 0 || blocks_copied < count) && !feof(input_file)) {
        if (ctrl_d) {
            ctrl_d = 0;
            print_dd_status_report();
        }

        size_t bytes_read = fread(buffer, 1, block_size, input_file);
        if (bytes_read == 0) {
            break;  
        }

        if (bytes_read == block_size) {
            full_blocks_in++;
        } else {
            partial_blocks_in++;
        }

        size_t bytes_write = fwrite(buffer, 1, bytes_read, output_file);

        if (bytes_write == block_size) {
            full_blocks_out++;
        } else {
            partial_blocks_out++;
        }

        total_bytes_copied += bytes_read;
        blocks_copied++;
    }
    free(buffer);
}

int main(int argc, char **argv) {
    clock_gettime(CLOCK_MONOTONIC, &start);
    signal(SIGUSR1, signal_handler);
    parse_arg(argc, argv);

    my_dd();
    print_dd_status_report();
    fclose(input_file);
    fclose(output_file);

    return 0;
}
