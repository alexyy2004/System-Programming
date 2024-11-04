/**
 * charming_chatroom
 * CS 341 - Fall 2024
 */
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>  // For struct addrinfo
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdint.h>  // For intptr_t
#include <sys/types.h>
#include <unistd.h>


#include "utils.h"

#define MAX_CLIENTS 8

void *process_client(void *p);

static volatile int serverSocket;
static volatile int endSession;

static volatile int clientsCount;
static volatile int clients[MAX_CLIENTS];

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Signal handler for SIGINT.
 * Used to set flag to end server.
 */
void close_server() {
    endSession = 1;
    // add any additional flags here you want.
}

/**
 * Cleanup function called in main after `run_server` exits.
 * Server ending clean up (such as shutting down clients) should be handled
 * here.
 */
void cleanup() {
    if (shutdown(serverSocket, SHUT_RDWR) != 0) {
        perror("shutdown():");
    }
    close(serverSocket);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1) {
            if (shutdown(clients[i], SHUT_RDWR) != 0) {
                perror("shutdown(): ");
            }
            close(clients[i]);
        }
    }
}

/**
 * Sets up a server connection.
 * Does not accept more than MAX_CLIENTS connections.  If more than MAX_CLIENTS
 * clients attempts to connects, simply shuts down
 * the new client and continues accepting.
 * Per client, a thread should be created and 'process_client' should handle
 * that client.
 * Makes use of 'endSession', 'clientsCount', 'client', and 'mutex'.
 *
 * port - port server will run on.
 *
 * If any networking call fails, the appropriate error is printed and the
 * function calls exit(1):
 *    - fprtinf to stderr for getaddrinfo
 *    - perror() for any other call
 */
// void run_server(char *port) {
//     /*QUESTION 1*/
//     /*QUESTION 2*/
//     /*QUESTION 3*/

//     /*QUESTION 8*/

//     /*QUESTION 4*/
//     /*QUESTION 5*/
//     /*QUESTION 6*/

//     /*QUESTION 9*/

//     /*QUESTION 10*/

//     /*QUESTION 11*/
// }

void run_server(char *port) {
    struct addrinfo hints, *res;
    int new_client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t thread_id;

    // Initialize server socket
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP

    if (getaddrinfo(NULL, port, &hints, &res) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(errno));
        exit(1);
    }

    serverSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (serverSocket == -1) {
        perror("socket");
        freeaddrinfo(res);
        exit(1);
    }

    int optval = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("setsockopt");
        close(serverSocket);
        freeaddrinfo(res);
        exit(1);
    }

    if (bind(serverSocket, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind");
        close(serverSocket);
        freeaddrinfo(res);
        exit(1);
    }

    freeaddrinfo(res);

    if (listen(serverSocket, MAX_CLIENTS) == -1) {
        perror("listen");
        close(serverSocket);
        exit(1);
    }

    printf("Server listening on port %s\n", port);

    while (!endSession) {
        new_client_fd = accept(serverSocket, (struct sockaddr *)&client_addr, &client_len);
        if (new_client_fd == -1) {
            if (errno == EINTR && endSession) {
                break;  // Graceful exit on signal interrupt
            }
            perror("accept");
            continue;
        }

        pthread_mutex_lock(&mutex);
        if (clientsCount < MAX_CLIENTS) {
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] == -1) {
                    clients[i] = new_client_fd;
                    clientsCount++;
                    pthread_create(&thread_id, NULL, process_client, (void *)(intptr_t)i);
                    break;
                }
            }
        } else {
            // Reject client if MAX_CLIENTS limit reached
            fprintf(stderr, "Max clients reached. Rejecting new connection.\n");
            close(new_client_fd);
        }
        pthread_mutex_unlock(&mutex);
    }
}


/**
 * Broadcasts the message to all connected clients.
 *
 * message  - the message to send to all clients.
 * size     - length in bytes of message to send.
 */
void write_to_clients(const char *message, size_t size) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1) {
            ssize_t retval = write_message_size(size, clients[i]);
            if (retval > 0) {
                retval = write_all_to_socket(clients[i], message, size);
            }
            if (retval == -1) {
                perror("write(): ");
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}

/**
 * Handles the reading to and writing from clients.
 *
 * p  - (void*)intptr_t index where clients[(intptr_t)p] is the file descriptor
 * for this client.
 *
 * Return value not used.
 */
void *process_client(void *p) {
    pthread_detach(pthread_self());
    intptr_t clientId = (intptr_t)p;
    ssize_t retval = 1;
    char *buffer = NULL;

    while (retval > 0 && endSession == 0) {
        retval = get_message_size(clients[clientId]);
        if (retval > 0) {
            buffer = calloc(1, retval);
            retval = read_all_from_socket(clients[clientId], buffer, retval);
        }
        if (retval > 0)
            write_to_clients(buffer, retval);

        free(buffer);
        buffer = NULL;
    }

    printf("User %d left\n", (int)clientId);
    close(clients[clientId]);

    pthread_mutex_lock(&mutex);
    clients[clientId] = -1;
    clientsCount--;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "%s <port>\n", argv[0]);
        return -1;
    }

    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = close_server;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    run_server(argv[1]);
    cleanup();
    pthread_exit(NULL);
}
