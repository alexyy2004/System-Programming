// An example server for local development and testing purposes which you are free to use - you do not need to submit this code
// clang -o server -O0 -Wall -Wextra -Werror -g -std=c99 -D_GNU_SOURCE -DDEBUG server.c
   
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>

#define MAX_CONTENT 256
#define DIFFICULT_PROBABILITY_PERCENT 50
#define STUBBON_MODULO 4


void error_exit(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

char to_mode(char*s) {
  if(!s) return 0;
  if(!strcmp("basic",s)) return 'b';
  if(!strcmp("difficult",s)) return 'd';
  if(!strcmp("stubborn",s)) return 's';
  return 0;
}


int main(int argc, char *argv[]) {
    char mode = 'x';
    int port = 0;
    if (argc != 4 || !(mode = to_mode(argv[1])) || !(port=atoi(argv[2]))) {
        fprintf(stderr, "Usage: %s  [basic|difficult|stubborn] <port> <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    signal(SIGPIPE, SIG_IGN);
    // Open and read the file into memory
    const char *filename = argv[3];
    int fd = open(filename, O_RDONLY);
    if (fd < 0) error_exit("Failed to open file");

    struct stat file_stat;
    if (fstat(fd, &file_stat) < 0) error_exit("Failed to stat file");

    size_t file_size = file_stat.st_size;
    char *file_data = malloc(file_size);
    if (!file_data) error_exit("Failed to allocate memory");

    if (read(fd, file_data, file_size) != (ssize_t) file_size)
        error_exit("Failed to read file into memory");

    close(fd);

    // Initialize random number generator
    srand(time(NULL));

    // Set up the server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) error_exit("Failed to create socket");

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        error_exit("Failed to set socket options");

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port),
    };

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        error_exit("Failed to bind socket");

    if (listen(server_fd, 5) < 0) error_exit("Failed to listen on socket");

    printf("Server listening on port %d\n", port);
    uint32_t stubborn_index = rand() & 0xff;

    // Accept and handle client connections
    while (1) {
 
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Failed to accept connection");
            continue;
        }
   
        // Handle client request
        uint32_t offset_net;
        ssize_t bytes_received = recv(client_fd, &offset_net, sizeof(offset_net), 0);
        if (bytes_received != sizeof(offset_net)) {
            perror("Failed to read offset from client");
            close(client_fd);
            continue;
        }
        // A difficult server will occasionally just close the connection without any reply. When this happens the client should wait for one second and then reconnect and try the same request again.
        if (mode =='d' && (rand() % 100) < DIFFICULT_PROBABILITY_PERCENT) {
          printf("Difficult server closing the connection!\n");
          close(client_fd);
          continue;
        }

        uint32_t offset = ntohl(offset_net); // Convert from network to host byte order
        
        uint32_t response_length = (offset >= file_size) ? 0 : file_size - offset;
        if( response_length > MAX_CONTENT)  response_length = 1 + (rand() % MAX_CONTENT);
        
        // A stubborn server is trickier; it returns just the 4 byte length and refuse to return any content for a few specific (non-zero) offset values. An intelligent client will adjust the request accordingly.
        // Offset 0 always works
        if(mode =='s' && offset && ((offset + stubborn_index) % STUBBON_MODULO) == 0) {
          printf("Stubborn server returning no content bytes\n");
          close(client_fd);
          continue;
        }

        uint32_t file_size_net = htonl(file_size); // Convert file size to network byte order
        char response[sizeof(uint32_t) + MAX_CONTENT];
        memcpy(response, &file_size_net, sizeof(uint32_t));
        memcpy(response + sizeof(uint32_t), file_data + offset, response_length);

        ssize_t bytes_to_send = sizeof(uint32_t) + response_length;
        if (send(client_fd, response, bytes_to_send, 0) != bytes_to_send) {
            perror("Failed to send response to client");
        } else {
            printf("Sent %u bytes of content to client\n", response_length);
        }

        close(client_fd);
    }

    // Clean up
    free(file_data);
    close(server_fd);
    return 0;
}
