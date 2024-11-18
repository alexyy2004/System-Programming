/**
 * nonstop_networking
 * CS 341 - Fall 2024
 */
#pragma once
#include <stddef.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include "format.h"

#define LOG(...)                      \
    do {                              \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n");        \
    } while (0);

typedef enum { GET, PUT, DELETE, LIST, V_UNKNOWN } verb;

ssize_t read_from_socket(int socket, char *buffer, size_t size);