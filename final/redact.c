// author: yueyan2
// ChatGPT gave me some starting code but it crashes, so I added comments and starting fixing it.
// C is hard. #NeedANewJob

// At least it compiles now! There's a few warnings and it doesn't actually work but I did the hard part for you.

// Also I started to make some test data (unzip chats.zip) to check it works with 1000s of files .... but I havent used it yet.
// e.g. ./redact test chats/small1 chats/small2 

// For future: Testing idea from stackoverflow: To reduce number of max open fds: ulimit -Sn 100
// but what that means or does, or why I should care I have no idea.
// Todo: Find out about canonical and symbolic link stuff
// I'm going home at lunch. If I don't come back it is not my problem any more. Good luck reader!
// And please replace my name in the author line. I don't want to be associated with this travesty of code anymore.

// p.s. Don't hate me because of this code, hate me because I borrowed (permanently) your favorite hoody you left on your chair 
// And ate your lunch in the fridge for a snack - Thanks (but less spice next time okay?)
// Byyee from your warm (and now ex) co-worker. 
// pps. I may need a letter of recommendation from you.

// I'm compiling like this (or -o hello)
// clang -o redact -pthread -O0 -Wall -Wextra -Werror -g -std=c99 -D_GNU_SOURCE -DDEBUG redact.c

// Some includes. There's a lot of them. Do I really need these. Python import from is so much simpler.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>


// Some constants
// A second comment in case anyone thinks that extra comments are useful.
// This sets BUFFER_SIZE and MAX_PATH_LEN. see comments can be so insightful and helpful, that's why I write uncommented python code.
#define BUFFER_SIZE 32
#define MAX_PATH_LEN 1024

// Thanks ChatGPT. This is a bit like a python class declaration (probably)

typedef struct {
    char *paths[BUFFER_SIZE];
    int start, end, count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty, not_full;
} RingBuffer;

// Why do we need "*" everywhere. Pointers? Huh. Why you make my life so complicated?
// In other news I think C hates me. Or at least keeps me on my toes - like bad slippers

typedef struct {
    char *mode;
    RingBuffer *buffer;
    int *file_count;
    int *total_size;
    int *total_digits;
    pthread_mutex_t *stats_mutex;
} ConsumerArgs;

// Now we make one.
RingBuffer ring_buffer = {
    .start = 0, .end = 0, .count = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .not_empty = PTHREAD_COND_INITIALIZER,
    .not_full = PTHREAD_COND_INITIALIZER,
};

// The stats we care abuot
int files_found = 0, total_size = 0, total_digits = 0;

// Mutex. Yay. Hmm Hey at least I googled that. What more do you want?

pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

// Finally some code I can understand. 
void print_usage() {
  // Gotta change it to standard error
  // puts(standarderror,"blah") wont compile :-( ?
    fprintf(stderr, "Usage: ./redact [test|modify] dir1 dir2 ...\n");
    exit(1); // Quick exit (like me)
}

// Thanks ChatGPT for this magic. What is a ring buffer anyway?
// Some kind of Lord of the Ring or Vampire reference?
// what is strdup? I feel duped!! :-) Sigh. No one gets my jokes.
// Should rename this to enrage for the LOLs.
void enqueue(RingBuffer *buffer, const char *path) {
    pthread_mutex_lock(&buffer->mutex);
    while (buffer->count == BUFFER_SIZE)
        pthread_cond_wait(&buffer->not_full, &buffer->mutex);
    // buffer->paths[buffer->end] = strdup(path);
    if (path != NULL) {
        buffer->paths[buffer->end] = strdup(path);
    } else {
        buffer->paths[buffer->end] = NULL;
    }
    buffer->end = (buffer->end + 1) % BUFFER_SIZE;
    buffer->count++;
    pthread_cond_signal(&buffer->not_empty);
    pthread_mutex_unlock(&buffer->mutex);
}
// Opposite of enqueue. It removes an item. (I think).
// Kinda seems complicated to me, in javascript I can just use unshift.  
// Wish I was paid by the line count.
// Should rename this to deranged for more LOLs.
char *dequeue(RingBuffer *buffer) {
    pthread_mutex_lock(&buffer->mutex);
    while (buffer->count == 0)
        pthread_cond_wait(&buffer->not_empty, &buffer->mutex);
    char *path = buffer->paths[buffer->start];
    // Path of least resistantance Ha ha.
    buffer->start = (buffer->start + 1) % BUFFER_SIZE;
    buffer->count--;
    pthread_cond_signal(&buffer->not_full);
    pthread_mutex_unlock(&buffer->mutex);
    return path;
}

void *producer(void *arg) {
    char *dir_path = (char *)arg; // What?
    DIR *dir = opendir(dir_path);
    if (!dir) { // Huh how does this work.
      // Todo standard error. And this crashes. 
      // Apart from that I can safely safe it compiles and is the greatest C code I have ever written or will ever write again.
        fprintf(stderr, "%s:%s\n", dir_path, strerror(errno)); // where is errno set? What voodoo is this?
        exit(2);
    }
    // Todo : What is a canonical path? This code isnt working yet, not sure why. I should have stuck to python.
    struct dirent *entry;
    char full_path[MAX_PATH_LEN];
    struct stat st;
    while ((entry = readdir(dir))) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        // Use lstat to check if regular file and not a symlink
        if (lstat(full_path, &st) == 0) {
            size_t len = strlen(entry->d_name);
            if (S_ISREG(st.st_mode) && (len > 4 && strcmp(entry->d_name + len - 4, ".txt") == 0)) {
                // Get canonical path
                char real_path[MAX_PATH_LEN];
                if (!realpath(full_path, real_path)) {
                    continue;
                }

                printf("FOUND %s\n", real_path);
                // Put the ring buffer stuff here
                enqueue(&ring_buffer, real_path);

                pthread_mutex_lock(&stats_mutex);
                files_found++;
                pthread_mutex_unlock(&stats_mutex);
            }
        }
    }
    closedir(dir);
    return NULL;
}

void process_file(const char *filepath, const char *mode, int *digits, int *size) {
    int fd = open(filepath, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "%s:%s\n", filepath, strerror(errno));
        exit(99);
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        fprintf(stderr, "%s:%s\n", filepath, strerror(errno));
        close(fd);
        exit(99);
    }

    if (st.st_size > 0) {
        char *data = mmap(NULL, st.st_size, mode[0] == 'm' ? PROT_READ | PROT_WRITE : PROT_READ, MAP_SHARED, fd, 0);
        if (data == MAP_FAILED) {
            fprintf(stderr, "%s:%s\n", filepath, strerror(errno));
            close(fd);
            exit(99);
        }

        int local_digits = 0;
        for (off_t i = 0; i < st.st_size; i++) {
            if (data[i] >= '0' && data[i] <= '9') {
                local_digits++;
                if (mode[0] == 'm')
                    // Won't Compile!? data[i] =  "*";
                data[i] =  (char) '*';
            }
        }

        if (munmap(data, st.st_size) < 0) {
            fprintf(stderr, "%s:%s\n", filepath, strerror(errno));
            close(fd);
            exit(99);
        }

        *digits = local_digits;
        *size = (int)st.st_size;
    } else {
        // Empty file
        *digits = 0;
        *size = 0;
    }

    close(fd);
}

// Consume arguments.
void *consumer(void *arg) {
    ConsumerArgs *args = (ConsumerArgs *)arg;
    while (1) {
        // printf("start while loop\n");
        char *filepath = dequeue(args->buffer);
        // printf("CONSUMING111111 %s\n", filepath);
        if (!filepath) break;

        printf("CONSUMING %s\n", filepath);

        int file_digits = 0, file_size = 0;
        process_file(filepath, args->mode, &file_digits, &file_size);

        pthread_mutex_lock(args->stats_mutex);
        (*args->total_size) += file_size;
        (*args->total_digits) += file_digits;
        pthread_mutex_unlock(args->stats_mutex);

        printf("PROCESSED %s SIZE:%d Digits:%d\n", filepath, file_size, file_digits);
        free(filepath);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3 || (strcmp(argv[1],"test") != 0 && strcmp(argv[1],"modify") != 0)) {
        print_usage();
    }

    char *mode = argv[1];
    int num_dirs = argc - 2;

    // Thanks ChatGPT for making some threads for me
    pthread_t producers[num_dirs];
    for (int i = 0; i < num_dirs; i++) {
        if (pthread_create(&producers[i], NULL, producer, argv[i + 2]) != 0) {
            perror("Failed to create producer thread");
            exit(99);
        }
    }

    pthread_t consumers[4];
    
    // "&..." Are these like C++ references? Some synchronization magic I bet
    ConsumerArgs args = {
        .mode = mode,
        .buffer = &ring_buffer,
        .file_count = &files_found,
        .total_size = &total_size,
        .total_digits = &total_digits,
        .stats_mutex = &stats_mutex,
    };

    // Thanks GhatGPT for making 4 consumer threads for me - you're the best (or at least better than me)
    for (int i = 0; i < 4; i++) {
        if (pthread_create(&consumers[i], NULL, consumer, &args) != 0) {
            perror("Failed to create consumer thread");
            exit(1);
        }
    }
    // Todo: Now just wait for everything to do its magic.
    // This next bit is kinda not working. You might say it's broke but then you'd hurt my code's feelings and I don't know how to fix it.
    
    puts("Debug - Waiting for Producers to finish");
    for (int i = 0; i < num_dirs; i++) {
        pthread_join(producers[i], NULL);
    }
    
    for (int i = 0; i < 4; i++) {
        enqueue(&ring_buffer, NULL);
    }

    puts("Debug - Waiting for Consumers to finish");
    for (int i = 0; i < 4; i++) {
        pthread_join(consumers[i], NULL);
    }
    // I am so confused. What does pthread_join do anyway? Maybe I should join the Navy and get some structure in my life.
    // I'm lost at C! (That was another Sea v C joke, see?) Ho hum. I'm a standup code comic.
    puts("Debug - Signal consumers to finish");
    enqueue(&ring_buffer, NULL);   

    // But I never see this !?!? It never exits. I should never have left <insert home town>.
    printf("FINISHED Files:%d Total Size:%d Total Digits:%d\n", files_found, total_size, total_digits);
    return 0; // Yeah we're so done here.
}
