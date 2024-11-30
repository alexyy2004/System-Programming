/**
 * nonstop_networking
 * CS 341 - Fall 2024
 */
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "dictionary.h"
#include "format.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>

#define MAX_CLIENTS 8
#define MAX_EVENTS 100
static int epfd;


void ignore_signal() {
    // ignore SIGPIPE
}

void signal_handler() {

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

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
    {
        perror("setsockopt SO_REUSEADDR failed");
        close(sock_fd);
        exit(1);
    }

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(int)) < 0)
    {
        perror("setsockopt SO_REUSEPORT failed");
        close(sock_fd);
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
        close(sock_fd);
        exit(1);
    }

    if (listen(sock_fd, MAX_CLIENTS) != 0)
    {
        perror(NULL);
        // freeaddrinfo(res);
        close(sock_fd);
        exit(1);
    }
    freeaddrinfo(res);


    epfd = epoll_create(1);
	if (epfd == -1) {
        perror("epoll_create1()");
        exit(1);
    }
    struct epoll_event ev1 = {.events = EPOLLIN, .data.fd = sock_fd};
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock_fd, &ev1) == -1) {
        perror("epoll_ctl: sock_fd");
        exit(1);
    }
    while (1) {
        struct epoll_event array[MAX_EVENTS];
        int num_events = epoll_wait(epfd , array, MAX_EVENTS /* max events */, 1000 /* timeout ms*/);
        if (num_events == -1) {
            perror("epoll_wait");
            exit(1);
        }
        for (int i = 0; i < num_events; i++) {
            int fd = array[i].data.fd;
            if (fd == sock_fd) {
                int client_fd = accept(sock_fd, NULL, NULL);
                if (client_fd == -1) {
                    perror("accept");
                    exit(1);
                }
                struct epoll_event ev_client = {.events = EPOLLIN, .data.fd = client_fd};
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev_client) == -1) {
                    perror("epoll_ctl: client_fd");
                    close(client_fd);
                    exit(1);
                }
            } else {
                // process client
                char buffer[1024];
                ssize_t bytes_read = read(fd, buffer, 1024);
                if (bytes_read == -1) {
                    perror("read");
                    close(fd);
                    continue;
                }
                if (bytes_read == 0) {
                    close(fd);
                    continue;
                }
                write(fd, buffer, bytes_read);
            }
        }
    }
    // freeaddrinfo(res);
}

void close_server(int sig) {

}

int main(int argc, char **argv) {
    // good luck!
    signal(SIGPIPE, ignore_signal);
	if (argc != 2) {
        print_server_usage();
        exit(1);
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
}
