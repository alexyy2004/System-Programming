/**
 * mini_memcheck
 * CS 341 - Fall 2024
 */
#include "mini_memcheck.h"
#include <stdio.h>
#include <string.h>

meta_data* head;
size_t total_memory_requested = 0;
size_t total_memory_freed = 0;
size_t invalid_addresses = 0;

void *mini_malloc(size_t request_size, const char *filename,
                  void *instruction) {
    // your code here
    if (request_size == 0) {
        return NULL;
    }
    meta_data *new_meta_data = malloc(sizeof(meta_data) + request_size);
    if (new_meta_data == NULL) {
        return NULL;
    }
    new_meta_data->request_size = request_size;
    new_meta_data->filename = filename;
    new_meta_data->instruction = instruction;
    new_meta_data->next = head;
    head = new_meta_data;
    total_memory_requested += request_size;
    return (void *)new_meta_data + sizeof(meta_data);
}

void *mini_calloc(size_t num_elements, size_t element_size,
                  const char *filename, void *instruction) {
    // your code here
    if (num_elements == 0 || element_size == 0) {
        return NULL;
    }
    size_t request_size = num_elements * element_size;
    void *ptr = mini_malloc(request_size, filename, instruction);
    if (ptr == NULL) {
        return NULL;
    }
    memset(ptr, 0, request_size);
    return ptr;
}

void *mini_realloc(void *payload, size_t request_size, const char *filename,
                   void *instruction) {
    // your code here
    if (payload == NULL && request_size == 0) {
        return NULL;
    }
    if (payload == NULL) {
        return mini_malloc(request_size, filename, instruction);
    }
    if (request_size == 0) {
        mini_free(payload);
        return NULL;
    }
    
    meta_data *prev = NULL;
    meta_data *curr = head;
    while (curr != NULL) {
        if ((void *)curr + sizeof(meta_data) == payload) {
            break;
        }
        prev = curr;
        curr = curr->next;
    }
    if (curr == NULL) {
        invalid_addresses += 1;
        return NULL;
    }

    meta_data *new_meta_data = NULL;
    // if (new_meta_data == NULL) {
    //     return NULL;
    // }
    
    if (curr->request_size < request_size) {
        total_memory_requested = total_memory_requested + (request_size - curr->request_size);
        new_meta_data = realloc(curr, sizeof(meta_data) + request_size);
    } else {
        total_memory_freed = total_memory_freed + (curr->request_size - request_size);
        new_meta_data = realloc(curr, sizeof(meta_data) + request_size);
    }

    if (prev == NULL) {
        head = new_meta_data;
        // free(curr);
    } else {
        prev->next = new_meta_data;
        new_meta_data->next = curr->next;
        // free(curr);
    }

    new_meta_data->request_size = request_size;
    new_meta_data->filename = filename;
    new_meta_data->instruction = instruction;
    return (void *)new_meta_data + sizeof(meta_data);
}

void mini_free(void *payload) {
    // your code here
    if (payload == NULL) {
        return;
    }

    meta_data *prev = NULL;
    meta_data *curr = head;
    while (curr != NULL) {
        if ((void *)curr + sizeof(meta_data) == payload) {
            break;
        }
        prev = curr;
        curr = curr->next;
    }
    if (curr == NULL) {
        invalid_addresses += 1;
        return;
    }

    if (prev == NULL) {
        head = curr->next;
    } else {
        prev->next = curr->next;
    }

    total_memory_freed += curr->request_size;
    free(curr);
}
