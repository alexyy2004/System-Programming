/**
 * vector
 * CS 341 - Fall 2024
 */
#include "sstring.h"
#include "vector.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <string.h>

struct sstring {
    // Anything you want
    vector *vec;
};

sstring *cstr_to_sstring(const char *input) {
    // your code goes here
    sstring *str = calloc(1, sizeof(sstring));
    str->vec = char_vector_create();
    for (size_t i = 0; i < strlen(input); i++) {
        vector_push_back(str->vec, (void *)(input + i));
    }
    return str;
}

char *sstring_to_cstr(sstring *input) {
    // your code goes here
    char *cstr = calloc(vector_size(input->vec) + 1, 1);
    for (size_t i = 0; i < vector_size(input->vec); i++) {
        char *c = (char *)vector_get(input->vec, i);
        cstr[i] = *c;
    }
    cstr[vector_size(input->vec)] = '\0';
    return cstr;
}

int sstring_append(sstring *this, sstring *addition) {
    // your code goes here
    for (size_t i = 0; i < vector_size(addition->vec); i++) {
        vector_push_back(this->vec, vector_get(addition->vec, i));
    }
    return vector_size(this->vec);
}

vector *sstring_split(sstring *this, char delimiter) {
    // your code goes here
    vector *vec = string_vector_create();
    char *cstr = sstring_to_cstr(this);
    char *token = strsep(&cstr, &delimiter);
    // char *token = strtok(cstr, &delimiter);
    while (token != NULL) {
        vector_push_back(vec, token);
        // token = strtok(NULL, &delimiter);
        token = strsep(&cstr, &delimiter);
    }
    free(cstr);
    return vec;
}

int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    // your code goes here
    char *cstr = sstring_to_cstr(this);
    char *ptr = strstr(cstr + offset, target);
    if (ptr == NULL) {
        free(cstr);
        return -1;
    }
    size_t len = strlen(target);
    size_t sub_len = strlen(substitution);
    size_t new_len = strlen(cstr) - len + sub_len;
    char *new_cstr = calloc(new_len + 1, 1);
    strncpy(new_cstr, cstr, strlen(cstr) - strlen(ptr));
    strcat(new_cstr, substitution);
    strcat(new_cstr, ptr + len);
    free(cstr);
    vector_clear(this->vec);
    this->vec = string_vector_create();
    for (size_t i = 0; i < new_len; i++) {
        vector_push_back(this->vec, new_cstr + i);
    }
    free(new_cstr);
    return 0;
}

char *sstring_slice(sstring *this, int start, int end) {
    // your code goes here
    char *cstr = sstring_to_cstr(this);
    char *slice = calloc(end - start + 1, 1);
    strncpy(slice, cstr + start, end - start);
    slice[end - start] = '\0';
    free(cstr);
    return slice;
}

void sstring_destroy(sstring *this) {
    // your code goes here
    vector_destroy(this->vec);
    free(this);
}
