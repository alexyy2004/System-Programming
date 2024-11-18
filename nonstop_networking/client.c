// /**
//  * nonstop_networking
//  * CS 341 - Fall 2024
//  */
// #include "format.h"
// #include <ctype.h>
// #include <stdbool.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <unistd.h>
// #include <netdb.h>
// #include <errno.h>
// #include <fcntl.h>
// #include <sys/stat.h>

// #include "common.h"


// #define BUFFER_SIZE 1024
// #define MAX_HEADER 1024
// #define MAX_FILENAME 255

// char **parse_args(int argc, char **argv);
// verb check_args(char **args);
// int connect_to_server(const char *host, const char *port);
// void handle_request(int sockfd, verb command, char **args);

// /**
//  * host - Server to connect to.
//  * port - Port to connect to server on.
//  *
//  * Returns integer of valid file descriptor, or exit(1) on failure.
//  */
// int connect_to_server(const char *host, const char *port) {
//     struct addrinfo hints, *res;
//     int status;
//     memset(&hints, 0, sizeof hints);
//     hints.ai_family = AF_INET;        // Use IPv4
//     hints.ai_socktype = SOCK_STREAM;  // Use TCP
    
//     status = getaddrinfo(host, port, &hints, &res);
//     if (status != 0) {
//         fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
//         exit(1);
//     }

//     int sockfd = 0;
//     sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
//     if (sockfd == -1) {
//         perror("socket");
//         freeaddrinfo(res);
//         exit(1);
//     }

//     if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
//         perror("connect");
//         freeaddrinfo(res);
//         exit(1);
//     }

//     freeaddrinfo(res);
//     return sockfd;
// }

// size_t read_from_server(int sockfd, char *buffer, size_t length) {
//     size_t bytes_read = 0;
//     while (bytes_read < length) {
//         ssize_t n = read(sockfd, buffer + bytes_read, length - bytes_read);
//         if (n < 0) {
//             perror("read");
//             exit(EXIT_FAILURE);
//         } else if (n == 0) {
//             break; // Connection closed
//         }
//         bytes_read += n;
//     }
//     return bytes_read;
// }

// int write_to_server(int sockfd, char* buffer, size_t length) {
//     size_t bytes_written = 0;
//     while (bytes_written < length) {
//         int n = write(sockfd, buffer + bytes_written, strlen(buffer) - bytes_written);
//         if (n < 0) {
//             perror("write");
//             exit(EXIT_FAILURE);
//         }
//         bytes_written += n;
//     }
//     return bytes_written;
// }

// void handle_request(int sockfd, verb command, char **args) {
//     char buffer[BUFFER_SIZE];
//     memset(buffer, 0, BUFFER_SIZE);

//     switch (command) {
//         case LIST:
//             snprintf(buffer, MAX_HEADER, "LIST\n");
//             write_to_server(sockfd, buffer, strlen(buffer));

//             // Read and print server response
//             while ((read_from_server(sockfd, buffer, BUFFER_SIZE)) > 0) {
//                 printf("%s", buffer);
//                 memset(buffer, 0, BUFFER_SIZE);
//             }
//             break;

//         case GET: {
//             snprintf(buffer, MAX_HEADER, "GET %s\n", args[3]);
//             write_to_server(sockfd, buffer, strlen(buffer));

//             // Read server response
//             size_t bytes = read_from_server(sockfd, buffer, BUFFER_SIZE);
//             if (strncmp(buffer, "OK\n", 3) == 0) {
//                 // Write binary data to local file
//                 FILE *file = fopen(args[4], "wb");
//                 if (!file) {
//                     perror("fopen");
//                     exit(EXIT_FAILURE);
//                 }
//                 fwrite(buffer + 3, 1, bytes - 3, file);
//                 while ((bytes = read_from_server(sockfd, buffer, BUFFER_SIZE)) > 0) {
//                     fwrite(buffer, 1, bytes, file);
//                 }
//                 fclose(file);
//             } else {
//                 fprintf(stderr, "Error: %s\n", buffer);
//             }
//             break;
//         }

//         case DELETE: {
//             snprintf(buffer, MAX_HEADER, "DELETE %s\n", args[3]);
//             write_to_server(sockfd, buffer, strlen(buffer));
//             printf("%s", buffer);
//             print_success();
//             break;
//         }

//         case PUT: {
//             struct stat buf;
//             if(stat(args[4], &buf) == -1) {
//                 exit(1);
//             }    
//             size_t size = buf.st_size;
//             write_to_socket(sockfd, (char*)&size, sizeof(size_t));
//             FILE *file = fopen(args[4], "r");
//             if (!file) {
//                 perror("fopen");
//                 exit(EXIT_FAILURE);
//             }

//             size_t bytes_write = 0;
//             while (bytes_write < size) {
//                 ssize_t size_remain = 0;
//                 if ((size - bytes_write) > BUFFER_SIZE) {
//                     size_remain = BUFFER_SIZE;
//                 } else {
//                     size_remain = size - bytes_write;
//                 }
//                 char buffer[size_remain + 1];
//                 fread(buffer, 1, size_remain, file);
//                 if (write_to_socket(sockfd, buffer, size_remain) < size_remain) {
//                     print_connection_closed();
//                     exit(1);
//                 }
//                 bytes_write += size_remain;
//             }

//             fclose(file);

//             // Read server response
//             printf("%s", buffer);
//             print_success();
//             break;
//         }

//         default: {
//             size_t bytes_rd = read_from_server(sockfd, buffer, strlen("ERROR\n"));
//             read_from_socket(sockfd, buffer+bytes_rd, strlen("ERROR\n")-bytes_rd);
//             if (strcmp(buffer, "ERROR\n") == 0) {
//                 fprintf(stdout, "%s", buffer);
//                 char err[20] = {0};
//                 if (!read_from_server(sockfd, err, 20))
//                     print_connection_closed();
//                 print_error_message(err);
//             } else {
//                 print_invalid_response();
//             }
//         }
//     }
// }


// int main(int argc, char **argv) {
//     // Good luck!
//     char **args = parse_args(argc, argv); //char* array in form of {host, port, method, remote, local, NULL}
//     verb request = check_args(args);

//     int socketserver = connect_to_server(args[0], args[1]);
//     handle_request(socketserver, request, args);

//     close(socketserver);
//     free(args);

// }

// /**
//  * Given commandline argc and argv, parses argv.
//  *
//  * argc argc from main()
//  * argv argv from main()
//  *
//  * Returns char* array in form of {host, port, method, remote, local, NULL}
//  * where `method` is ALL CAPS
//  */
// char **parse_args(int argc, char **argv) {
//     if (argc < 3) {
//         return NULL;
//     }

//     char *host = strtok(argv[1], ":");
//     char *port = strtok(NULL, ":");
//     if (port == NULL) {
//         return NULL;
//     }

//     char **args = calloc(1, 6 * sizeof(char *));
//     args[0] = host;
//     args[1] = port;
//     args[2] = argv[2];
//     char *temp = args[2];
//     while (*temp) {
//         *temp = toupper((unsigned char)*temp);
//         temp++;
//     }
//     if (argc > 3) {
//         args[3] = argv[3];
//     }
//     if (argc > 4) {
//         args[4] = argv[4];
//     }

//     return args;
// }

// /**
//  * Validates args to program.  If `args` are not valid, help information for the
//  * program is printed.
//  *
//  * args     arguments to parse
//  *
//  * Returns a verb which corresponds to the request method
//  */
// verb check_args(char **args) {
//     if (args == NULL) {
//         print_client_usage();
//         exit(1);
//     }

//     char *command = args[2];

//     if (strcmp(command, "LIST") == 0) {
//         return LIST;
//     }

//     if (strcmp(command, "GET") == 0) {
//         if (args[3] != NULL && args[4] != NULL) {
//             return GET;
//         }
//         print_client_help();
//         exit(1);
//     }

//     if (strcmp(command, "DELETE") == 0) {
//         if (args[3] != NULL) {
//             return DELETE;
//         }
//         print_client_help();
//         exit(1);
//     }

//     if (strcmp(command, "PUT") == 0) {
//         if (args[3] == NULL || args[4] == NULL) {
//             print_client_help();
//             exit(1);
//         }
//         return PUT;
//     }

//     // Not a valid Method
//     print_client_help();
//     exit(1);
// }

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

// Function declarations
char **parse_args(int argc, char **argv);
verb check_args(char **args);
int connect_to_server(const char *host, const char *port);
void handle_request(int sockfd, verb command, char **args);
size_t read_from_socket(int sockfd, char *buffer, size_t length);
int write_to_socket(int sockfd, const char *buffer, size_t length);

/**
 * Connects to the server.
 */
int connect_to_server(const char *host, const char *port) {
    struct addrinfo hints, *res;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP

    status = getaddrinfo(host, port, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        perror("socket");
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        freeaddrinfo(res);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);
    return sockfd;
}

/**
 * Reads data from a socket.
 */
size_t read_from_socket(int sockfd, char *buffer, size_t length) {
    size_t bytes_read = 0;
    while (bytes_read < length) {
        ssize_t n = read(sockfd, buffer + bytes_read, length - bytes_read);
        if (n < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        } else if (n == 0) {
            break; // Connection closed
        }
        bytes_read += n;
    }
    return bytes_read;
}

/**
 * Writes data to a socket.
 */
int write_to_socket(int sockfd, const char *buffer, size_t length) {
    size_t bytes_written = 0;
    while (bytes_written < length) {
        ssize_t n = write(sockfd, buffer + bytes_written, length - bytes_written);
        if (n < 0) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        bytes_written += n;
    }
    return 0; // Success
}

/**
 * Handles client requests (LIST, GET, DELETE, PUT).
 */
void handle_request(int sockfd, verb command, char **args) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    switch (command) {
        case LIST:
            snprintf(buffer, MAX_HEADER, "LIST\n");
            write_to_socket(sockfd, buffer, strlen(buffer));

            while ((read_from_socket(sockfd, buffer, BUFFER_SIZE)) > 0) {
                printf("%s", buffer);
                memset(buffer, 0, BUFFER_SIZE);
            }
            break;

        case GET: {
            snprintf(buffer, MAX_HEADER, "GET %s\n", args[3]);
            write_to_socket(sockfd, buffer, strlen(buffer));

            size_t bytes = read_from_socket(sockfd, buffer, BUFFER_SIZE);
            if (strncmp(buffer, "OK\n", 3) == 0) {
                FILE *file = fopen(args[4], "wb");
                if (!file) {
                    perror("fopen");
                    exit(EXIT_FAILURE);
                }
                fwrite(buffer + 3, 1, bytes - 3, file);
                while ((bytes = read_from_socket(sockfd, buffer, BUFFER_SIZE)) > 0) {
                    fwrite(buffer, 1, bytes, file);
                }
                fclose(file);
            } else {
                fprintf(stderr, "Error: %s\n", buffer);
            }
            break;
        }

        case DELETE:
            snprintf(buffer, MAX_HEADER, "DELETE %s\n", args[3]);
            write_to_socket(sockfd, buffer, strlen(buffer));
            printf("%s", buffer);
            break;

        case PUT: {
            struct stat buf;
            if (stat(args[4], &buf) == -1) {
                perror("stat");
                exit(EXIT_FAILURE);
            }

            size_t file_size = buf.st_size;
            write_to_socket(sockfd, (char *)&file_size, sizeof(size_t));

            FILE *file = fopen(args[4], "rb");
            if (!file) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }

            size_t bytes_written = 0;
            while (bytes_written < file_size) {
                size_t chunk_size = (file_size - bytes_written > BUFFER_SIZE) ? BUFFER_SIZE : (file_size - bytes_written);
                fread(buffer, 1, chunk_size, file);
                write_to_socket(sockfd, buffer, chunk_size);
                bytes_written += chunk_size;
            }

            fclose(file);
            break;
        }

        default:
            fprintf(stderr, "Invalid command.\n");
            exit(EXIT_FAILURE);
    }
}

/**
 * Parses arguments from the command line.
 */
char **parse_args(int argc, char **argv) {
    if (argc < 3) {
        return NULL;
    }

    char *host = strtok(argv[1], ":");
    char *port = strtok(NULL, ":");
    if (!port) {
        return NULL;
    }

    char **args = calloc(6, sizeof(char *));
    args[0] = host;
    args[1] = port;
    args[2] = argv[2];
    for (char *p = args[2]; *p; p++) {
        *p = toupper(*p);
    }
    if (argc > 3) args[3] = argv[3];
    if (argc > 4) args[4] = argv[4];

    return args;
}

/**
 * Validates arguments and returns the corresponding command verb.
 */
verb check_args(char **args) {
    if (!args) {
        print_client_usage();
        exit(1);
    }

    if (strcmp(args[2], "LIST") == 0) return LIST;
    if (strcmp(args[2], "GET") == 0 && args[3] && args[4]) return GET;
    if (strcmp(args[2], "DELETE") == 0 && args[3]) return DELETE;
    if (strcmp(args[2], "PUT") == 0 && args[3] && args[4]) return PUT;

    print_client_help();
    exit(1);
}

/**
 * Main function.
 */
int main(int argc, char **argv) {
    char **args = parse_args(argc, argv);
    verb command = check_args(args);

    int sockfd = connect_to_server(args[0], args[1]);
    handle_request(sockfd, command, args);

    close(sockfd);
    free(args);
    return 0;
}
