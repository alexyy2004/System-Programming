/**
 * password_cracker
 * CS 341 - Fall 2024
 */
#include "cracker1.h"
#include "format.h"
#include "utils.h"
#include <pthread.h>
#include "includes/queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crypt.h>
#include <stdint.h>



typedef struct task {
    char *username;
    char *password_hash;
    char *known_password;
} task_t;

queue *task_queue = NULL;
int num_succeed = 0;
int num_failed = 0;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

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
    task_t *task = NULL;
    while ((task = queue_pull(task_queue))) {
        v1_print_thread_start(threadId, task->username);
        double start_time = getThreadCPUTime();
        double end_time = getThreadCPUTime();
        int hashCount = 0;
        char *hashed = NULL;
        char *pwd = strdup(task->known_password);
        setStringPosition(pwd + getPrefixLength(task->known_password), 0);
        bool flag = false;

        // generate by Chatgpt
        do {
            pthread_mutex_lock(&m);
            hashed = crypt_r(pwd, "xx", &cd);
            hashCount++;
            end_time = getThreadCPUTime();
            if (!strcmp(hashed, task->password_hash)) {
                flag = true;
                double total_time = end_time - start_time;
                v1_print_thread_result(threadId, task->username, pwd, hashCount, total_time, 0);
                num_succeed++;
                pthread_mutex_unlock(&m);
                break;
            }
            incrementString(pwd);
            if (strncmp(pwd, task->known_password, getPrefixLength(task->known_password))) {
                pthread_mutex_unlock(&m);
                break;
            }
            pthread_mutex_unlock(&m);
        } while (strcmp(pwd, task->known_password));

        free(pwd);
        if (!flag) {
            pthread_mutex_lock(&m);
            end_time = getThreadCPUTime();
            double total_time = end_time - start_time;
            v1_print_thread_result(threadId, task->username, NULL, hashCount, total_time, 1);
            num_failed++;
            pthread_mutex_unlock(&m);
        }
        destroy_task(task);
    }
    queue_push(task_queue, NULL);
    return NULL;
}


int start(size_t thread_count) {
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads
    // read in the input and create tasks
    task_queue = queue_create(-1);
    char *line = NULL;
    size_t len = 0;
    int read;
    
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
        task_t *task = create_task(username, password_hash, known_password);
        queue_push(task_queue, task);
    }


    queue_push(task_queue, NULL); // signal the end of the queue
    free(line);

    // create threads
    pthread_t threads[thread_count];
    for (size_t i = 0; i < thread_count; i++) {
        pthread_create(&threads[i], NULL, crack_passwords, (void *)(i+1));
    }
    for (size_t i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    queue_destroy(task_queue);
    v1_print_summary(num_succeed, num_failed);
    pthread_mutex_destroy(&m);
    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}

