/**
 * teaching_threads
 * CS 341 - Fall 2024
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "reduce.h"
#include "reducers.h"

/* You might need a struct for each task ... */
typedef struct {
    int *list;
    size_t start_idx;
    size_t end_idx;
    reducer reduce_func;
    int base_case;
    int result;
} ThreadArgs;

void *thread_reduce(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    int partial_result = args->base_case;
    for (size_t i = args->start_idx; i < args->end_idx; ++i) {
        partial_result = args->reduce_func(partial_result, args->list[i]);
    }
    args->result = partial_result;
    return NULL;
}

/* You should create a start routine for your threads. */
int par_reduce(int *list, size_t list_len, reducer reduce_func, int base_case,
               size_t num_threads) {
    /* Your implementation goes here */
    if (list_len == 0) {
        return base_case;
    }

    size_t threads_to_create = num_threads;
    if (threads_to_create > list_len) {
        threads_to_create = list_len;
    } 

    pthread_t *threads = malloc(sizeof(pthread_t) * threads_to_create);
    ThreadArgs *thread_args = malloc(sizeof(ThreadArgs) * threads_to_create);

    size_t size = list_len / threads_to_create;
    size_t remainder = list_len % threads_to_create;
    size_t start_idx = 0;

    for (size_t i = 0; i < threads_to_create; ++i) {
        size_t end_idx = start_idx + size;
        if (remainder > 0) {
            end_idx += 1;
            remainder -= 1;
        }
        thread_args[i].list = list;
        thread_args[i].start_idx = start_idx;
        thread_args[i].end_idx = end_idx;
        thread_args[i].reduce_func = reduce_func;
        thread_args[i].base_case = base_case;
        thread_args[i].result = base_case;
        pthread_create(&threads[i], NULL, thread_reduce, &thread_args[i]);
        start_idx = end_idx;
    }

    for (size_t i = 0; i < threads_to_create; ++i) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join failed");
            exit(EXIT_FAILURE);
        }
    }

    int result = base_case;
    for (size_t i = 0; i < threads_to_create; ++i) {
        result = reduce_func(result, thread_args[i].result);
    }

    free(threads);
    free(thread_args);
    return result;
}
