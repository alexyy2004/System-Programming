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
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>

#define MAX_CLIENTS 8
#define MAX_EVENTS 100
#define MAX_HEADER_LEN 1024
#define MAX_FILENAME 256
#define BUFFER_SIZE 1024

static int gloabl_epfd;
static dictionary *clients_dict; // maps client_fd to client_info
static char *global_temp_dir;
static vector *file_list;
static dictionary *global_file_size; // maps filename to file size

typedef struct client_info {
    // state = 0: header not parsed
    // state = 1: header parsed
    // state = -1: bad request
    // state = -2: bad file size
    // state = -3: no such file
    int state;
    verb command;
    char filename[MAX_FILENAME];
    char header[MAX_HEADER_LEN];
} client_info;



void ignore_signal() {
    // ignore SIGPIPE
}

void initialize_global_variables() { //todo
    clients_dict = int_to_shallow_dictionary_create();
    global_temp_dir = NULL;
    global_file_size = string_to_unsigned_long_dictionary_create();
    file_list = vector_create(string_copy_constructor, string_destructor, string_default_constructor);
}

int read_from_client(int client_fd, client_info *info) {
    int path_len = strlen(global_temp_dir) + 1 + strlen(info->filename) + 1;
    char file_path[path_len];
    memset(file_path, 0, path_len);
    sprintf(file_path, "%s/%s", global_temp_dir, info->filename);
    FILE *read_file = fopen(file_path, "r");
    FILE *write_file = fopen(file_path, "w");
    if (!write_file) {
        perror("fopen");
        exit(1);
    }
    size_t file_size;
    read_from_socket(client_fd, (char *)&file_size, sizeof(size_t));
    
    size_t byte_write = 0;
    while (byte_write < file_size) {
        ssize_t size_remain = 0;
        if ((file_size - byte_write) > BUFFER_SIZE) {
            size_remain = BUFFER_SIZE;
        } else {
            size_remain = file_size - byte_write;
        }
        char buffer_temp[size_remain + 1];
        memset(buffer_temp, 0, size_remain + 1);
        size_t read_size = read_from_socket(client_fd, buffer_temp, size_remain);
        fwrite(buffer_temp, 1, read_size, write_file);
        byte_write += read_size;
        if (read_size == 0) {
            break;
        }
    }
    
    // error check
    bool flag = false;
    if (byte_write == 0 && byte_write != file_size) {
        LOG("read_from_socket failed");
        print_connection_closed();
        flag = true;
    } else if (byte_write < file_size) {
        print_too_little_data();
        flag = true;
    } else if (byte_write > file_size) {
        print_received_too_much_data();
        flag = true;
    } else {
        // fprintf(stdout, "%zu%s", file_size, buffer_temp);
        fclose(write_file);
    }
    if (flag) {
        remove(file_path);
        return 1;
    }

    if (!read_file) {
        vector_push_back(file_list, info->filename);
    } else {
        fclose(read_file);
    }
    dictionary_set(global_file_size, info->filename, &file_size);
    return 0;
}

void read_header(int client_fd, client_info *info) {
    size_t bytes_read = 0;
    while (bytes_read < MAX_HEADER_LEN) {
        ssize_t read_size = read(client_fd, info->header + bytes_read, 1);
        if (read_size == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("read_header failed");
            exit(1);
        }
        if (read_size == 0 || info->header[strlen(info->header) - 1] == '\n') {
            break;
        }
        bytes_read++;
    }

    if (bytes_read == MAX_HEADER_LEN) { // parse header failed
        info->state = -1;
        struct epoll_event ev_out = {.data.fd = client_fd, .events = EPOLLOUT};
        epoll_ctl(gloabl_epfd, EPOLL_CTL_MOD, client_fd, &ev_out);
        return;
    } else { // parse header success
        LOG("info->header: %s", info->header);
        if (strcmp(info->header, "GET") == 0) {
            info->command = GET;
            strcpy(info->filename, info->header + 4);
            info->filename[strlen(info->filename) - 1] = '\0';
            info->state = 1;
            struct epoll_event ev_out = {.data.fd = client_fd, .events = EPOLLOUT};
            epoll_ctl(gloabl_epfd, EPOLL_CTL_MOD, client_fd, &ev_out);
        } else if (strcmp(info->header, "PUT") == 0) {
            info->command = PUT;
            strcpy(info->filename, info->header + 4);
            info->filename[strlen(info->filename) - 1] = '\0';
            if (read_from_client(client_fd, info)) { // bad file size
                info->state = -2;
                struct epoll_event ev_out = {.data.fd = client_fd, .events = EPOLLOUT};
                epoll_ctl(gloabl_epfd, EPOLL_CTL_MOD, client_fd, &ev_out);
                return;
            }
            info->state = 1;
            struct epoll_event ev_out = {.data.fd = client_fd, .events = EPOLLOUT};
            epoll_ctl(gloabl_epfd, EPOLL_CTL_MOD, client_fd, &ev_out);
        } else if (strcmp(info->header, "DELETE") == 0) { 
            info->command = DELETE;
            strcpy(info->filename, info->header + 7);
            info->filename[strlen(info->filename) - 1] = '\0';
            info->state = 1;
            struct epoll_event ev_out = {.data.fd = client_fd, .events = EPOLLOUT};
            epoll_ctl(gloabl_epfd, EPOLL_CTL_MOD, client_fd, &ev_out);
        } else if (strcmp(info->header, "LIST") == 0) {
            info->command = LIST;
            info->state = 1;
            struct epoll_event ev_out = {.data.fd = client_fd, .events = EPOLLOUT};
            epoll_ctl(gloabl_epfd, EPOLL_CTL_MOD, client_fd, &ev_out);
        } else { // invalid response
            info->state = -1;
            struct epoll_event ev_out = {.data.fd = client_fd, .events = EPOLLOUT};
            epoll_ctl(gloabl_epfd, EPOLL_CTL_MOD, client_fd, &ev_out);
            print_invalid_response();
            return;
        }
    }
}

int process_DELETE(int client_fd, client_info *info) {
    int path_len = strlen(global_temp_dir) + 1 + strlen(info->filename) + 1;
    char file_path[path_len];
    memset(file_path, 0, path_len);
    sprintf(file_path, "%s/%s", global_temp_dir, info->filename);
    if (remove(file_path) == -1) { // no such file
        info->state = -3;
        return 1;
    }
    for (size_t i = 0; i < vector_size(file_list); i++) {
        if (strcmp(vector_get(file_list, i), info->filename) == 0) {
            vector_erase(file_list, i);
            break;
        }
    }
    
    dictionary_remove(global_file_size, info->filename);
    return 0;
}

int process_LIST(int client_fd, client_info *info) {
    size_t file_size = 0;
    if (write_to_socket(client_fd, "OK\n", 3) != 3) {
        return 1;
    }
    for (size_t i = 0; i < vector_size(file_list); i++) {
        file_size = file_size + strlen(vector_get(file_list, i)) + 1;
    }
    if (file_size != 0) {
        file_size -= 1;
    }
    
    write_to_socket(client_fd, (char *)&file_size, sizeof(size_t));
    for (size_t i = 0; i < vector_size(file_list); i++) {
        write_to_socket(client_fd, vector_get(file_list, i), strlen(vector_get(file_list, i)));
        if (i != (vector_size(file_list) - 1)) {
            write_to_socket(client_fd, "\n", 1);
        }
    }
    return 0;
}

int process_GET(int client_fd, client_info *info) {
    int filename_len = strlen(global_temp_dir) + 1 + strlen(info->filename) + 1; // `%s/%s`
    char filepath[filename_len];
    memset(filepath, 0, filename_len);
    sprintf(filepath, "%s/%s", global_temp_dir, info->filename); // Populate the filepath

    FILE* read_file = fopen(filepath, "r");
    if (read_file == NULL) {
        info->state = -3; // No such file error
        return 1;
    }

    write_to_socket(client_fd, "OK\n", 3); 
    if (strlen(info->filename) == 0) {
        info->state = -1; 
        return 1;
    }

    size_t size = *(size_t *)dictionary_get(global_file_size, info->filename);
    write_to_socket(client_fd, (char *)&size, sizeof(size_t));

    size_t bytes_write = 0;
    while (bytes_write < size) {
        size_t new_bytes_write = 0;
        if (size - bytes_write > MAX_HEADER_LEN) {
            new_bytes_write = MAX_HEADER_LEN;
        } else {
            new_bytes_write = size - bytes_write;
        }
        char buffer[MAX_HEADER_LEN + 1];
        memset(buffer, 0, MAX_HEADER_LEN + 1);
        fread(buffer, 1, new_bytes_write, read_file);
        write_to_socket(client_fd, buffer, new_bytes_write);
        bytes_write += new_bytes_write;
    }

    fclose(read_file);
    return 0;
}

void clean_client(int client_fd) {
	client_info *info = dictionary_get(clients_dict, &client_fd);
	free(info);
	epoll_ctl(gloabl_epfd, EPOLL_CTL_DEL, client_fd, NULL);
	
	dictionary_remove(clients_dict, &client_fd);
	shutdown(client_fd, SHUT_RDWR);
	close(client_fd);
}

void process_cmd(int client_fd, client_info *info) {
    if (info->command == PUT) {
        LOG("process PUT");
        write_to_socket(client_fd, "OK\n", 3); // already checked in read_header
        LOG("process PUT end");
    }

    if (info->command == GET) { //todo
        int result = process_GET(client_fd, info);
        if (result == 1) {
            return;
        }
    }
 
    if (info->command == DELETE) { 
        int result = process_DELETE(client_fd, info);
        if (result == 1) { // delete failed
            return; // return to error_handler
        }
        write_to_socket(client_fd, "OK\n", 3);
    }

    if (info->command == LIST) {
        int result = process_LIST(client_fd, info);
        if (result == 1) {
            return;
        }
    }
    clean_client(client_fd);
}

void error_handler(int client_fd, client_info *info) {

}

void run_client(int client_fd) {
	client_info *info = dictionary_get(clients_dict, &client_fd);
	if (info->state == 0) {
		read_header(client_fd, info);
	} else if (info->state == 1) {
		process_cmd(client_fd, info);
	} else {
		error_handler(client_fd, info);
	}

    // bool error = true;
    // if (info->state == 0) {
    //     read_header(client_fd, info);
    // }
    // if (info->state == 1) {
    //     process_cmd(client_fd, info);
    //     error = false;
    // }
    // if (error) {
    //     error_handler(client_fd, info);
    // }
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
void run_server(char *port) {
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


    gloabl_epfd = epoll_create(1);
	if (gloabl_epfd == -1) {
        perror("epoll_create failed");
        exit(1);
    }
    struct epoll_event ev1 = {.events = EPOLLIN, .data.fd = sock_fd};
    if (epoll_ctl(gloabl_epfd, EPOLL_CTL_ADD, sock_fd, &ev1) == -1) {
        perror("epoll_ctl: sock_fd failed");
        exit(1);
    }
    while (1) {
        struct epoll_event array[MAX_EVENTS];
        int num_events = epoll_wait(gloabl_epfd , array, MAX_EVENTS /* max events */, 1000 /* timeout ms*/);
        if (num_events == -1) {
            perror("epoll_wait failed");
            exit(1);
        }
        for (int i = 0; i < num_events; i++) {
            int fd = array[i].data.fd;
            if (fd == sock_fd) { // this is the server socket
                int client_fd = accept(sock_fd, NULL, NULL);
                if (client_fd == -1) {
                    perror("accept failed");
                    exit(1);
                }
                struct epoll_event ev_client = {.events = EPOLLIN, .data.fd = client_fd};
                // make the client socket non-blocking
                int flags = fcntl(client_fd, F_GETFL, 0);
                if (flags == -1) {
                    perror("fcntl F_GETFL failed");
                }
                flags |= O_NONBLOCK;
                if (fcntl(client_fd, F_SETFL, flags) == -1) {
                    perror("fcntl F_SETFL failed");
                }
  
                if (epoll_ctl(gloabl_epfd, EPOLL_CTL_ADD, client_fd, &ev_client) == -1) {
                    perror("epoll_ctl: client_fd failed");
                    close(client_fd);
                    exit(1);
                }
            } else { // this is the client socket
                run_client(fd);
            }
        }
    }
    // freeaddrinfo(res);
}

void close_server(int sig) {
    remove(global_temp_dir);
    vector *infos = dictionary_values(clients_dict);
    for (size_t i = 0; i < vector_size(infos); i++) {
        free(vector_get(infos, i));
    }
    vector_destroy(infos);
    dictionary_destroy(clients_dict);

    dictionary_destroy(global_file_size);
    vector_destroy(file_list);
    close(gloabl_epfd);
    exit(1);
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
        exit(1);
    }

    char template[] = "XXXXXX";
    global_temp_dir = mkdtemp(template); 
    print_temp_directory(global_temp_dir);

    initialize_global_variables();
    run_server(argv[1]);
}
