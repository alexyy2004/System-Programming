/**
 * charming_chatroom
 * CS 341 - Fall 2024
 * 
 * [Group Working]
 * Group Member Netids: pjame2, boyangl3, yueyan2
 * 
 * [AI Usage]
 * Referenced ChatGPT on how to use SO_REUSEADDR and SO_REUSEPORT.
 */
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

#define MAX_CLIENTS 8

void *process_client(void *p);

static volatile int serverSocket;
static volatile int endSession;

static volatile int clientsCount;
static volatile int clients[MAX_CLIENTS];

// static int num_threads;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Signal handler for SIGINT.
 * Used to set flag to end server.
 */
void close_server()
{
    endSession = 1;
    // add any additional flags here you want.
    // if (shutdown(serverSocket, SHUT_RDWR) == -1)
    // {
    //     perror(NULL);
    //     exit(1);
    // }

    // if (close(serverSocket) == -1)
    // {
    //     perror(NULL);
    //     exit(1);
    // }
    // exit(0);
}

/**
 * Cleanup function called in main after `run_server` exits.
 * Server ending clean up (such as shutting down clients) should be handled
 * here.
 */
void cleanup()
{
    if (shutdown(serverSocket, SHUT_RDWR) != 0)
    {
        perror("shutdown():");
    }
    close(serverSocket);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] != -1)
        {
            if (shutdown(clients[i], SHUT_RDWR) != 0)
            {
                perror("shutdown(): ");
            }
            close(clients[i]);
        }
    }
}

int find_free()
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] <= 0)
        {
            printf("Found free client at %d\n", i);
            return i;
        }
    }
    return -1;
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
void run_server(char *port)
{
    /*QUESTION 1*/
    int s;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1)
    {
        perror("socket_failed");
        exit(1);
    }

    int reuse = 1;

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("setsockopt SO_REUSEADDR failed");
        exit(1);
    }

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0)
    {
        perror("setsockopt SO_REUSEPORT failed");
        exit(1);
    }

    struct addrinfo hints;
    struct addrinfo *res = NULL;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    s = getaddrinfo(NULL, port, &hints, &res);
    if (s != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(1);
    }

    if (bind(sock_fd, res->ai_addr, res->ai_addrlen) != 0)
    {
        perror(NULL);
        // freeaddrinfo(res);
        exit(1);
    }

    if (listen(sock_fd, MAX_CLIENTS) != 0)
    {
        perror(NULL);
        // freeaddrinfo(res);
        exit(1);
    }

    serverSocket = sock_fd;
    freeaddrinfo(res);

    pthread_t threads[MAX_CLIENTS];
    size_t i = 0;
    for (; i < MAX_CLIENTS; i++) {
        clients[i] = -1;
    }
    while (1)
    {
        if (endSession == 1)
        {
            break;
        }
        printf("Waiting for connection...\n");
        int client_fd = accept(sock_fd, NULL, NULL);
        
        if (client_fd == -1) {
            perror(NULL);
            // freeaddrinfo(res);
            exit(1);
        }
        
        if (clientsCount >= MAX_CLIENTS)
        {
            // Shut down and close the fd of current client, shutdown client_fd
            if (shutdown(client_fd, SHUT_RDWR) == -1)
            {
                perror(NULL);
                // freeaddrinfo(res);
                exit(1);
            }

            if (close(client_fd) == -1)
            {
                perror(NULL);
                // freeaddrinfo(res);
                exit(1);
            }
            continue;
        }
        pthread_mutex_lock(&mutex);
        clientsCount++;
        int idx = find_free();
        // if (idx >= num_threads)
        // {
        //     num_threads = idx;
        // }
        if (idx == -1)
        {
            printf("Messed up someone else!");
        }
        clients[idx] = client_fd;

        printf("Client %d connected\n", idx);
        pthread_create(threads + idx, NULL, process_client, (void *)(intptr_t)idx);
        // if (endSession == 1)
        // {
        //     for (int i = 0; i < num_threads; i++) {
        //         pthread_join(threads[i], NULL);
        //     }
        //     break;
        // }
        pthread_mutex_unlock(&mutex);
    }
    // freeaddrinfo(res);
    
}

/**
 * Broadcasts the message to all connected clients.
 *
 * message  - the message to send to all clients.
 * size     - length in bytes of message to send.
 */
void write_to_clients(const char *message, size_t size)
{
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] != -1)
        {
            ssize_t retval = write_message_size(size, clients[i]);
            if (retval > 0)
            {
                retval = write_all_to_socket(clients[i], message, size);
            }
            if (retval == -1)
            {
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
void *process_client(void *p)
{
    pthread_detach(pthread_self());
    intptr_t clientId = (intptr_t)p;
    ssize_t retval = 1;
    char *buffer = NULL;

    while (retval > 0 && endSession == 0)
    {
        retval = get_message_size(clients[clientId]);
        if (retval > 0)
        {
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

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "%s <port>\n", argv[0]);
        return -1;
    }

    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = close_server;
    if (sigaction(SIGINT, &act, NULL) < 0)
    {
        perror("sigaction");
        return 1;
    }

    run_server(argv[1]);
    cleanup();
    pthread_exit(NULL);
}
