/**
 * charming_chatroom
 * CS 341 - Fall 2024
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   // For read() and write()
#include <errno.h>    // For errno and EINTR

#include "utils.h"
static const size_t MESSAGE_SIZE_DIGITS = 4;

char *create_message(char *name, char *message) {
    int name_len = strlen(name);
    int msg_len = strlen(message);
    char *msg = calloc(1, msg_len + name_len + 4);
    sprintf(msg, "%s: %s", name, message);

    return msg;
}

ssize_t get_message_size(int socket) {
    int32_t size;
    ssize_t read_bytes =
        read_all_from_socket(socket, (char *)&size, MESSAGE_SIZE_DIGITS);
    if (read_bytes == 0 || read_bytes == -1)
        return read_bytes;

    return (ssize_t)ntohl(size);
}

// You may assume size won't be larger than a 4 byte integer
// ssize_t write_message_size(size_t size, int socket) {
//     // Your code here
//     return 9001;
// }

ssize_t write_message_size(size_t size, int socket) {
    // Convert size to network byte order
    int32_t net_size = htonl((int32_t)size);

    // Write the size to the socket
    return write_all_to_socket(socket, (const char *)&net_size, MESSAGE_SIZE_DIGITS);
}


// ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
//     // Your Code Here
//     return 9001;
// }

ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
    size_t total_read = 0;
    ssize_t bytes_read;

    while (total_read < count) {
        bytes_read = read(socket, buffer + total_read, count - total_read);
        if (bytes_read == 0) {
            // Connection closed
            return total_read;
        } else if (bytes_read == -1) {
            if (errno == EINTR) {
                // Interrupted by signal, retry reading
                continue;
            } else {
                // Actual error
                perror("read");
                return -1;
            }
        }

        total_read += bytes_read;
    }

    return total_read;
}


// ssize_t write_all_to_socket(int socket, const char *buffer, size_t count) {
//     // Your Code Here
//     return 9001;
// }
ssize_t write_all_to_socket(int socket, const char *buffer, size_t count) {
    size_t total_written = 0;
    ssize_t bytes_written;

    while (total_written < count) {
        bytes_written = write(socket, buffer + total_written, count - total_written);
        if (bytes_written == -1) {
            if (errno == EINTR) {
                // Interrupted by signal, retry writing
                continue;
            } else {
                // Actual error
                perror("write");
                return -1;
            }
        }

        total_written += bytes_written;
    }

    return total_written;
}

