/**
 * password_cracker
 * CS 341 - Fall 2024
 */
#include "cracker2.h"
#include "format.h"
#include "utils.h"
#include <pthread.h>
#include "includes/queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crypt.h>
#include <stdint.h>

// debug with chatgpt and copilot

typedef struct task {
    char *username;
    char *password_hash;
    char *known_password;
} task_t;

queue *task_queue = NULL;
int num_succeed = 0;
int num_failed = 0;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

pthread_barrier_t barrier;
task_t *global_task = NULL;
int flag = 0; // 0 for success, 1 for failure
int hash_num = 0;
char *password = NULL; // cracked password
size_t global_thread_count = 0;

// destroy task
void destroy_task(task_t *task) {
    free(task->username);
    free(task->password_hash);
    free(task->known_password);
    free(task);
}

// create task
task_t *create_task(char *username, char *password_hash, char *known_password) {
    task_t *task = malloc(sizeof(task_t));
    task->username = strdup(username);
    task->password_hash = strdup(password_hash);
    task->known_password = strdup(known_password);
    return task;
}

// crack_passwords
void *crack_passwords(void *pid) {
    struct crypt_data cd;
    cd.initialized = 0;
    int threadId = (int)(intptr_t)pid;
    // task_t *task = NULL;
    char *pwd = NULL;

    while (1) {
        pthread_barrier_wait(&barrier);
        if (flag) {
            break;
        }
        long offset = 0;
        long count = 0;
        int known_letter = getPrefixLength(global_task->known_password);
        int unknown_letter = 8 - known_letter;
        pwd = strdup(global_task->known_password);

        getSubrange(unknown_letter, global_thread_count, threadId, &offset, &count);
        setStringPosition(pwd + known_letter, offset);
        v2_print_thread_start(threadId, global_task->username, offset, pwd);
        
        int hashCount = 0;
        char *hashed = NULL;
        for (long i = 0; i < count; i++) {
            hashed = crypt_r(pwd, "xx", &cd);
            hashCount++;
            // end_time = getThreadCPUTime();
            if (!strcmp(hashed, global_task->password_hash)) {
                pthread_mutex_lock(&m);
                strcpy(password, pwd);
                flag = 1;
                v2_print_thread_result(threadId, hashCount, 0);
                // num_succeed++;
                hash_num += hashCount;
                pthread_mutex_unlock(&m);
                break;
            }
            if (flag) {
                pthread_mutex_lock(&m);
                v2_print_thread_result(threadId, hashCount, 1);
                hash_num += hashCount;
                pthread_mutex_unlock(&m);
                break;
            }
            incrementString(pwd);
            // if (strncmp(pwd, task->known_password, getPrefixLength(task->known_password))) {
            //     // pthread_mutex_unlock(&m);
            //     break;
            // }
            // pthread_mutex_unlock(&m);
        } 

        if (!flag) {
            pthread_mutex_lock(&m);
            // end_time = getThreadCPUTime();
            // double total_time = end_time - start_time;
            v2_print_thread_result(threadId, hashCount, 2);
            // num_failed++;
            hash_num += hashCount;
            pthread_mutex_unlock(&m);
        }
        // destroy_task(task);
        pthread_barrier_wait(&barrier);
        // free(pwd);
    }
    // queue_push(task_queue, NULL);
    free(pwd);
    return NULL;
}

int start(size_t thread_count) {
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads
    // read in the input and create tasks
    pthread_mutex_init(&m, NULL);
    pthread_barrier_init(&barrier, NULL, thread_count+1);
    global_thread_count = thread_count;
    // task_queue = queue_create(-1);
    pthread_t threads[thread_count];
    for (size_t i = 0; i < thread_count; i++) {
        pthread_create(&threads[i], NULL, crack_passwords, (void *)(i+1));
    }
    char *line = NULL;
    size_t len = 0;
    int read;

    // global_task = calloc(1, sizeof(task_t));
    // global_task->username = calloc(8+1, sizeof(char));
    // global_task->password_hash = calloc(13+1, sizeof(char));
    // global_task->known_password = calloc(8+1, sizeof(char));
    password = calloc(9, sizeof(char));

    while ((read = getline(&line, &len, stdin)) != -1) {        
        if (read > 0 && line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }
        char *username = strtok(line, " ");
        char *password_hash = strtok(NULL, " ");
        char *known_password = strtok(NULL, " ");
        if (username == NULL || password_hash == NULL || known_password == NULL) {
            continue;
        }
        global_task = create_task(username, password_hash, known_password);
        // strcpy(global_task->username, username);
        // strcpy(global_task->password_hash, password_hash);
        // strcpy(global_task->known_password, known_password);
        
        pthread_barrier_wait(&barrier); // wait for all threads to be ready
        double start_time = getTime();
        double cpu_start_time = getCPUTime();
        v2_print_start_user(global_task->username);
        // pthread_barrier_wait(&barrier); // wait for all threads to be ready
        
        pthread_barrier_wait(&barrier); // wait for all threads to finish
        double end_time = getTime();
        double cpu_end_time = getCPUTime();
        double total_time = end_time - start_time;
        double total_cpu_time = cpu_end_time - cpu_start_time;
        int result = -1;
        if (flag) {
            result = 0;
        } else {
            result = 1;
        }
        v2_print_summary(global_task->username, password, hash_num, total_time, total_cpu_time, result);
        
        // queue_push(task_queue, task);
        // start new task
        pthread_mutex_lock(&m);
        flag = 0;
        hash_num = 0;
        pthread_mutex_unlock(&m);
    }


    // queue_push(task_queue, NULL); // signal the end of the queue
    flag = 1;
    destroy_task(global_task);
    free(line);
    free(password);
    pthread_barrier_wait(&barrier); // wait for all threads to finish

    for (size_t i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    // queue_destroy(task_queue);
    // v1_print_summary(num_succeed, num_failed);
    // pthread_mutex_destroy(&m);
    pthread_mutex_destroy(&m);
    pthread_barrier_destroy(&barrier);
    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}