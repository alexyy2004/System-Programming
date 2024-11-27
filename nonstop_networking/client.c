/**
 * nonstop_networking
 * CS 341 - Fall 2024
 */
#include "format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "common.h"


#define BUFFER_SIZE 1024
#define MAX_HEADER 1024
#define MAX_FILENAME 255

char **parse_args(int argc, char **argv);
verb check_args(char **args);
int connect_to_server(const char *host, const char *port);
void handle_request(int sockfd, verb command, char **args);

/**
 * host - Server to connect to.
 * port - Port to connect to server on.
 *
 * Returns integer of valid file descriptor, or exit(1) on failure.
 * use the connect_to_server from charming_chatroom lab, which collaborates with pjame2, boyangl3
 */
int connect_to_server(const char *host, const char *port) {
    struct addrinfo hints, *res;
    int status;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;        // Use IPv4
    hints.ai_socktype = SOCK_STREAM;  // Use TCP
    
    status = getaddrinfo(host, port, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    int sockfd = 0;
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        perror("socket");
        freeaddrinfo(res);
        exit(1);
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        freeaddrinfo(res);
        exit(1);
    }

    freeaddrinfo(res);
    return sockfd;
}      

void read_from_server(char **args, int sockfd, verb command) {
    char* buffer = calloc(1, strlen("OK\n") + 1);
    size_t byte_read = read_from_socket(sockfd, buffer, strlen("OK\n"));
    // fprintf(stdout, "balsjdblasbdjlbawlbd\n");
    // fprintf(stdout, "%d", strcmp(buffer, "OK\n"));
    
    if (strcmp(buffer, "OK\n") == 0) {
        fprintf(stdout, "%s", buffer);
        // LOG("OK");
        free(buffer);
        if (command == DELETE || command == PUT) {
            print_success();
        } else {
            if (command == LIST) {
                size_t file_size;
                read_from_socket(sockfd, (char *)&file_size, sizeof(size_t));
                char buffer_temp[file_size+2];
                memset(buffer_temp, 0, file_size+2);
                byte_read = read_from_socket(sockfd, buffer_temp, file_size+1);
                // fprintf(stdout, "%zu%s", file_size, buffer_temp);
                // error check
                if (byte_read == 0 && byte_read != file_size) {  
                    // fprintf(stderr, "byte_read: %zu, file_size: %zu", byte_read, file_size);
                    LOG("byte_read: %zu, file_size: %zu", byte_read, file_size);
                    print_connection_closed();
                    exit(1);
                } else if (byte_read < file_size) {

                    print_too_little_data();
                    exit(1);
                } else if (byte_read > file_size) {
                    print_received_too_much_data();
                    exit(1);
                } else {
                    fprintf(stdout, "%zu%s", file_size, buffer_temp);
                }
            }
            if (command == GET) { //todo
                FILE *file = fopen(args[4], "w");
                if (!file) {
                    perror("fopen");
                    exit(1);
                }
                size_t file_size;
                read_from_socket(sockfd, (char *)&file_size, sizeof(size_t));
                
                size_t byte_write = 0;
                while (byte_write < file_size) {
                    ssize_t size_remain = 0;
                    if ((file_size - byte_write) > BUFFER_SIZE) {
                        size_remain = BUFFER_SIZE;
                    } else {
                        size_remain = file_size - byte_write;
                    }
                    char buffer_temp[size_remain + 1];
                    if (read_from_socket(sockfd, buffer_temp, size_remain) < size_remain) {
                        LOG("read_from_socket failed");
                        print_connection_closed();
                        exit(1);
                    }
                    fwrite(buffer_temp, 1, size_remain, file);
                    byte_write += size_remain;
                }
                

                // error check
                if (byte_write == 0 && byte_write != file_size) {  
                    // fprintf(stderr, "byte_read: %zu, file_size: %zu", byte_read, file_size);
                    LOG("byte_read: %zu, file_size: %zu", byte_write, file_size);
                    print_connection_closed();
                    exit(1);
                } else if (byte_write < file_size) {
                    print_too_little_data();
                    exit(1);
                } else if (byte_write > file_size) {
                    print_received_too_much_data();
                    exit(1);
                } else {
                    // fprintf(stdout, "%zu%s", file_size, buffer_temp);
                    fclose(file);
                }
            }
        }
    } else {
        buffer = realloc(buffer, strlen("ERROR\n") + 1);
        read_from_socket(sockfd, buffer + strlen("OK\n"), strlen("ERROR\n") - strlen("OK\n"));
        // LOG("buffer: %s", buffer);
        // LOG("strcmp: %d", strcmp(buffer, "ERROR\n"));   
        if (strcmp(buffer, "ERROR\n") == 0) {
            fprintf(stdout, "%s", buffer);
            char err[20] = {0};
            if (!read_from_socket(sockfd, err, 20))
                print_connection_closed();
            print_error_message(err);
        } else {
            print_invalid_response();
        }
        free(buffer);
    }
    // free(buffer);
}

int write_to_server(int sockfd, char* buffer, size_t length) {
    size_t bytes_written = 0;
    while (bytes_written < length) {
        int n = write(sockfd, buffer + bytes_written, strlen(buffer) - bytes_written);
        if (n < 0) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        bytes_written += n;
    }
    return bytes_written;
}

void handle_request(int sockfd, verb command, char **args) {
    char* buffer = NULL;
    switch (command) {
        case LIST:
            buffer = calloc(1, 6);
            snprintf(buffer, MAX_HEADER, "LIST\n");
            if (write_to_socket(sockfd, buffer, 6) < 6) {
                print_connection_closed();
                exit(1);
            }

            if (shutdown(sockfd, SHUT_WR) != 0) {
                perror("shutdown()");
            }
            free(buffer);

            read_from_server(args, sockfd, command);
            break;

        case GET: { //todo
            buffer = calloc(1, strlen(args[2])+strlen(args[3])+3);
            snprintf(buffer, MAX_HEADER, "GET %s\n", args[3]);
            ssize_t bytes_written = write_to_socket(sockfd, buffer, strlen(buffer));
            if ((size_t)bytes_written < strlen(buffer)) {
                print_connection_closed();
                exit(1);
            }
            free(buffer);
            if (shutdown(sockfd, SHUT_WR) != 0) {
                perror("shutdown()");
            }

            read_from_server(args, sockfd, command);
            break;
        }

        case DELETE: { // need to check
            buffer = calloc(1, strlen(args[2])+strlen(args[3])+3);
            snprintf(buffer, MAX_HEADER, "DELETE %s\n", args[3]);
            ssize_t bytes_written = write_to_socket(sockfd, buffer, strlen(buffer));
            if ((size_t)bytes_written < strlen(buffer)) {
                print_connection_closed();
                exit(1);
            }
            free(buffer);
            if (shutdown(sockfd, SHUT_WR) != 0) {
                perror("shutdown()");
            }

            read_from_server(args, sockfd, command);
            break;
        }

        case PUT: {
            buffer = calloc(1, strlen(args[2])+strlen(args[3])+3);
            snprintf(buffer, MAX_HEADER, "PUT %s\n", args[3]);
            // write_to_socket(sockfd, buffer, strlen(buffer));
            // free(buffer);
            ssize_t bytes_written = write_to_socket(sockfd, buffer, strlen(buffer));
            ssize_t real_size = strlen(buffer);
            if (bytes_written < real_size) {
                LOG("!!!!!!write_to_socket failed");
                free(buffer);
                print_connection_closed();
                exit(1);
            }
            free(buffer);

            struct stat buf;
            if(stat(args[4], &buf) == -1) {
                exit(1);
            }    
            size_t size = buf.st_size;
            write_to_socket(sockfd, (char*)&size, sizeof(size_t));
            FILE *file = fopen(args[4], "r");
            if (!file) {
                perror("fopen");
                exit(1);
            }

            size_t bytes_write = 0;
            while (bytes_write < size) {
                ssize_t size_remain = 0;
                if ((size - bytes_write) > BUFFER_SIZE) {
                    size_remain = BUFFER_SIZE;
                } else {
                    size_remain = size - bytes_write;
                }
                char buffer[size_remain + 1];
                fread(buffer, 1, size_remain, file);
                if (write_to_socket(sockfd, buffer, size_remain) < size_remain) {
                    LOG("write_to_socket failed");
                    print_connection_closed();
                    exit(1);
                }
                bytes_write += size_remain;
            }
            fclose(file);

            if (shutdown(sockfd, SHUT_WR) != 0) {
                perror("shutdown()");
            }

            // Read server response
            read_from_server(args, sockfd, command);
            break;
        }

        default: { //todo
            buffer = calloc(1, strlen(args[2])+strlen(args[3])+3);
            snprintf(buffer, MAX_HEADER, args[2], args[3]);
            ssize_t bytes_written = write_to_socket(sockfd, buffer, strlen(buffer));
            if ((size_t)bytes_written < strlen(buffer)) {
                print_connection_closed();
                exit(1);
            }
            free(buffer);
            if (shutdown(sockfd, SHUT_WR) != 0) {
                perror("shutdown()");
            }

            read_from_server(args, sockfd, command);
        }
    }
    // free(buffer);
}


int main(int argc, char **argv) {
    // Good luck!
    char **args = parse_args(argc, argv); //char* array in form of {host, port, method, remote, local, NULL}
    verb request = check_args(args);

    int socketserver = connect_to_server(args[0], args[1]);
    handle_request(socketserver, request, args);
    shutdown(socketserver, SHUT_RD);
    close(socketserver);
    free(args);

}

/**
 * Given commandline argc and argv, parses argv.
 *
 * argc argc from main()
 * argv argv from main()
 *
 * Returns char* array in form of {host, port, method, remote, local, NULL}
 * where `method` is ALL CAPS
 */
char **parse_args(int argc, char **argv) {
    if (argc < 3) {
        return NULL;
    }

    char *host = strtok(argv[1], ":");
    char *port = strtok(NULL, ":");
    if (port == NULL) {
        return NULL;
    }

    char **args = calloc(1, 6 * sizeof(char *));
    args[0] = host;
    args[1] = port;
    args[2] = argv[2];
    char *temp = args[2];
    while (*temp) {
        *temp = toupper((unsigned char)*temp);
        temp++;
    }
    if (argc > 3) {
        args[3] = argv[3];
    }
    if (argc > 4) {
        args[4] = argv[4];
    }

    return args;
}

/**
 * Validates args to program.  If `args` are not valid, help information for the
 * program is printed.
 *
 * args     arguments to parse
 *
 * Returns a verb which corresponds to the request method
 */
verb check_args(char **args) {
    if (args == NULL) {
        print_client_usage();
        exit(1);
    }

    char *command = args[2];

    if (strcmp(command, "LIST") == 0) {
        return LIST;
    }

    if (strcmp(command, "GET") == 0) {
        if (args[3] != NULL && args[4] != NULL) {
            return GET;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "DELETE") == 0) {
        if (args[3] != NULL) {
            return DELETE;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "PUT") == 0) {
        if (args[3] == NULL || args[4] == NULL) {
            print_client_help();
            exit(1);
        }
        return PUT;
    }

    // Not a valid Method
    print_client_help();
    exit(1);
}
