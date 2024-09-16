/**
 * vector
 * CS 341 - Fall 2024
 */
#include "vector.h"
#include <stdio.h>
int main(int argc, char *argv[]) {
    // Write your test cases here
    vector *v = char_vector_create();
    char *str = "hello world!";
    char *ptr = str;

    while (*ptr) {
        printf("%c\n", *ptr);
        vector_push_back(v, ptr);
        vector_pop_back(v);
        vector_push_back(v, ptr);
        ptr++;
    }
    ptr = str;


    //
    printf("capacity : %zu\n", vector_capacity(v));
    vector_reserve(v, 5);
    printf("capacity : %zu\n", vector_capacity(v));

    printf("capacity : %zu\n", vector_capacity(v));
    printf("size : %zu\n", vector_size(v));
    for (size_t i = 0; i < vector_size(v); i++) {
      char *c = vector_get(v, i);
      printf("%s\n", c);
    }
    printf("\n");

    vector_reserve(v, 20);
    printf("vector_reserve(v, 20);\n");
    printf("capacity : %zu\n", vector_capacity(v));
    printf("size : %zu\n", vector_size(v));
    for (size_t i = 0; i < vector_size(v); i++) {
      char *c = (char *) vector_get(v, i);
      printf("%zu:%c;", i, *c);
    }
    printf("\n");

    vector_resize(v, 11);
    printf("vector_resize(v, 11);\n");
    printf("capacity : %zu\n", vector_capacity(v));
    printf("size : %zu\n", vector_size(v));
    for (size_t i = 0; i < vector_size(v); i++) {
      char *c = (char *) vector_get(v, i);
      printf("%zu:%c;\n", i, *c);
    }
    printf("\n");

    // following has segmenation fault
    vector_resize(v, 20);
    printf("vector_resize(v, 20);\n"); // resize 变大了，但是没有初始化
    printf("capacity 4: %zu\n", vector_capacity(v));
    printf("size 4: %zu\n", vector_size(v));
    for (size_t i = 0; i < vector_size(v); i++) {
      char *c = (char *) vector_get(v, i);
      printf("%zu:%c;\n", i, *c);
    }
    printf("\n");
    
    for (size_t i = 0; i < vector_size(v); i++) {
      char *c = (char *) vector_get(v, i);
      printf("%zu:%c;", i, *c);
    }
    // segmenation fault

    vector_destroy(v);
    return 0;
}
