/**
 * nonstop_networking
 * CS 341 - Fall 2024
 */
#include "common.h"

ssize_t read_from_socket(int sockfd, char *buffer, size_t length) {
    size_t total_bytes_read = 0; // Tracks the total bytes read
    while (total_bytes_read < length) {
        ssize_t bytes_read = read(sockfd, buffer + total_bytes_read, length - total_bytes_read);

        if (bytes_read < 0) {
            if (errno == EINTR) {
                // Interrupted by a signal, retry the read
                continue;
            }
            perror("read"); // Log the error
            return -1;      // Return -1 to indicate failure
        }

        if (bytes_read == 0) {
            // Connection closed by the peer
            break;
        }

        total_bytes_read += bytes_read;
    }

    return total_bytes_read; // Return the total bytes read
}