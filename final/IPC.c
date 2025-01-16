#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main() {
    int fd[2];
    char buffer[100];
    pipe(fd);

    if (fork() == 0) { // Child process
        close(fd[0]); // Close unused read end
        char *message = "Hello from child!";
        write(fd[1], message, strlen(message) + 1);
        close(fd[1]);
    } else { // Parent process
        close(fd[1]); // Close unused write end
        read(fd[0], buffer, sizeof(buffer)); 
        printf("Parent received: %s\n", buffer);
        close(fd[0]);
    }
    return 0;
}
