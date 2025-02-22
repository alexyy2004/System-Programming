/**
 * nonstop_networking
 * CS 341 - Fall 2024
 */
#include "common.h"
// use the write_to_socket from charming_chatroom lab, which collaborates with pjame2, boyangl3
ssize_t read_from_socket(int sockfd, char *buffer, size_t length) {
    size_t total_bytes_read = 0; // Tracks the total bytes read
    while (total_bytes_read < length) {
        ssize_t bytes_read = read(sockfd, buffer + total_bytes_read, length - total_bytes_read);
        // LOG("bytes_read in common: %zd", bytes_read);
        if (bytes_read < 0) {
            if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
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
        // LOG("first_total_bytes_read in common: %zd", total_bytes_read);
        total_bytes_read += bytes_read;
        // LOG("then_total_bytes_read in common: %zu", total_bytes_read);
    }

    return total_bytes_read; // Return the total bytes read
}


ssize_t write_to_socket(int socket, const char *buffer, size_t count)
{
    // Your Code Here
    size_t total_write = 0;
    while (total_write < count)
    {
        ssize_t write_result = write(socket, buffer + total_write, count - total_write);
        if (write_result == 0)
        { // Finished. Break and return
            return total_write;
        }
        else if (write_result > 0)
        { // Add bytes to total
            total_write += write_result;
        }
        else if (write_result < 0)
        { // Failure, check if error was interrupted
            if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
            { // error was interrupted
                continue;
            }
            else
            { // error was NOT interrupted
                return -1;
            }
        }
    }
    return total_write;
}

