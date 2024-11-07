/**
 * charming_chatroom
 * CS 341 - Fall 2024
 *
 * [Group Working]
 * Group Member Netids: pjame2, boyangl3, yueyan2
 *
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "utils.h"
static const size_t MESSAGE_SIZE_DIGITS = 4;

char *create_message(char *name, char *message)
{
    int name_len = strlen(name);
    int msg_len = strlen(message);
    char *msg = calloc(1, msg_len + name_len + 4);
    sprintf(msg, "%s: %s", name, message);

    return msg;
}

ssize_t get_message_size(int socket)
{
    int32_t size;
    ssize_t read_bytes =
        read_all_from_socket(socket, (char *)&size, MESSAGE_SIZE_DIGITS);
    if (read_bytes == 0 || read_bytes == -1)
        return read_bytes;

    return (ssize_t)ntohl(size);
}

// You may assume size won't be larger than a 4 byte integer
ssize_t write_message_size(size_t size, int socket)
{
    // Your code here
    size_t new_size = htonl(size);
    return write_all_to_socket(socket, (char *)&new_size, MESSAGE_SIZE_DIGITS);
}

ssize_t read_all_from_socket(int socket, char *buffer, size_t count)
{
    // Your Code Here
    ssize_t total_read = 0;
    while (total_read < (ssize_t)count)
    {
        ssize_t read_result = read(socket, buffer + total_read, count - total_read);
        if (read_result == 0)
        { // Finished. Break and return
            return total_read;;
        }
        else if (read_result > 0)
        { // Add bytes to total
            total_read += read_result;
        }
        else if (read_result < 0)
        { // Failure, check if error was interrupted
            if (errno != EINTR)
            { // error was NOT interrupted
                return -1;
            }
        }
    }
    return total_read;
}

ssize_t write_all_to_socket(int socket, const char *buffer, size_t count)
{
    // Your Code Here
    ssize_t total_write = 0;
    while (total_write < (ssize_t)count)
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
            if (errno != EINTR)
            { // error was NOT interrupted
                return -1;
            }
        }
    }
    return total_write;
}
