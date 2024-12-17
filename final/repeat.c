// author: yueyan2

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>


const char *server_ip;
int server_port;
size_t file_size = 0;
int file_size_known = 0;
char *file_data = NULL;

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

int write_request_to_server(int sock, uint32_t offset) {
    uint32_t net_offset = htonl(offset);
    ssize_t sent = send(sock, &net_offset, sizeof(net_offset), 0);
    if (sent != sizeof(net_offset)) {
        return -1;
    }
    return 0;
}

ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
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

// return -1 = connect failed; return 0 = read failed; return >0 = got content bytes
int request(uint32_t offset, char *buffer, size_t *buffer_size) {
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", server_port);
    int sock = connect_to_server(server_ip, port_str);
    if (sock < 0) {
        return -1;
    }

    if (write_request_to_server(sock, offset) < 0) {
        close(sock);
        return 0;
    }

    char buffer_temp[260];
    ssize_t read_bytes = read_all_from_socket(sock, buffer_temp, 260);
    close(sock);

    if (read_bytes < 4) {
        return 0;
    }

    uint32_t net_size;
    memcpy(&net_size, buffer_temp, 4);
    uint32_t actual_size = ntohl(net_size);

    if (!file_size_known) {
        file_size_known = 1;
        file_size = actual_size;
        if (file_size > 0) {
            file_data = malloc(file_size);
        }
    }

    int content_bytes = (int)(read_bytes - 4);
    memcpy(buffer, buffer_temp + 4, content_bytes);
    // printf("buffer: %s\n", buffer);
    *buffer_size = (size_t)content_bytes;

    return content_bytes;
}

size_t get_file_size() {
    size_t received_bytes = 0;
    while (!file_size_known) {
        char buffer[256];
        size_t buffer_size = 0;
        int res = request(0, buffer, &buffer_size);
        if (res < 0) { // connect failed
            return -1;
        } else{
            if (file_size_known) {
                if (file_size == 0) {
                    return 0; // read nothing
                } else {
                    // printf("buffer_size: %ld\n", buffer_size);
                    // printf("buffer: %s\n", buffer);
                    memcpy(file_data + received_bytes, buffer, buffer_size);
                    received_bytes += buffer_size;
                }
                // if (res > 0) {
                //     received_bytes += buffer_size;
                // }
            } else {
                sleep(1);
            }
        }
    }
    return received_bytes;
}

int download_file(char *download_filename) {
    FILE *out_file = fopen(download_filename, "wb");
    if (!out_file) {
        perror("fopen failed");
        return 1;
    }

    size_t receive_bytes = get_file_size();
    if ((int)receive_bytes == -1) {
        perror("connect failed");
        fclose(out_file);
        exit(2);
    }

    if ((int)receive_bytes == 0) {
        fclose(out_file);
        free(file_data);
        return 0;
    }
    // printf("file size: %ld\n", file_size);
    // printf("file data: %s\n", file_data);
    while (receive_bytes < file_size) {
        uint32_t offset = (uint32_t)receive_bytes;
        char buffer[256];
        size_t buffer_size = 0;

        int res = request(offset, buffer, &buffer_size);
        if (res < 0) {
            perror("connect failed");
            fclose(out_file);
            free(file_data);
            exit(2);
        }

        if (res == 0) { // read failed, retry
            int retry_count = 3;
            int res_temp = res;
            while (retry_count > 0 && res_temp == 0) {
                sleep(1);
                res_temp = request(offset, buffer, &buffer_size);
                if (res_temp < 0) {
                    perror("connect failed");
                    fclose(out_file);
                    free(file_data);
                    exit(2);
                }
                retry_count--;
            }

            if (res_temp == 0) { // still read failed, change offset
                int found_data = 0;
                uint32_t new_offset = offset - 1;
                int new_res = res_temp;
                int num_of_retries = 3;
                size_t new_buffer_len = 0;
                while (num_of_retries > 0 && new_res == 0) {
                    sleep(1);
                    new_res = request(new_offset, buffer, &new_buffer_len);
                    if (new_res < 0) {
                        perror("connect failed");
                        fclose(out_file);
                        free(file_data);
                        exit(2);
                    }
                    if (new_res > 0) {
                        found_data = 1;
                        break;
                    }
                    num_of_retries--;
                }
                if (new_res > 0) {
                    size_t start_new = receive_bytes - new_offset;
                    if (start_new < new_buffer_len) {
                        size_t new_count = new_buffer_len - start_new;
                        if (receive_bytes + new_count > file_size) {
                            new_count = file_size - receive_bytes;
                        }
                        memcpy(file_data + receive_bytes, buffer + start_new, new_count);
                        receive_bytes += new_count;
                    }
                }
                if (!found_data) {
                    sleep(1);
                    continue;
                }
            } else { // change offset success, then read
                size_t new_count = buffer_size;
                if (receive_bytes + new_count > file_size) {
                    new_count = file_size - receive_bytes;
                }
                memcpy(file_data + receive_bytes, buffer, new_count);
                receive_bytes += new_count;
            }
        } else { // read success
            size_t new_count = buffer_size;
            if (receive_bytes + new_count > file_size) {
                new_count = file_size - receive_bytes;
            }
            memcpy(file_data + receive_bytes, buffer, new_count);
            receive_bytes += new_count;
        }

        sleep(1);
    }

    if (receive_bytes > 0) {
        fwrite(file_data, 1, receive_bytes, out_file);
    }
    // printf("receive_bytes: %ld\n", receive_bytes);
    fclose(out_file);
    free(file_data);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    server_ip = argv[1];
    server_port = atoi(argv[2]);
    if (server_port <= 0) {
        fprintf(stderr, "Invalid port\n");
        exit(1);
    }
    
    char *file_name = basename(argv[0]);
    if (strcmp(file_name, "hello") == 0) {
        printf("hello world\n");
        exit(0);
    }

    char *download_filename = getenv("download");
    if (!download_filename) {
        exit(1);
    }

    return download_file(download_filename);
}



