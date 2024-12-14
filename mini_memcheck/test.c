/**
 * mini_memcheck
 * CS 341 - Fall 2024
 */
#include <stdlib.h>
int main() {
    void *p1 = malloc(30);
    void *p2 = malloc(40);
    void *p3 = malloc(50);
    void *p4 = realloc(p2, 60);
    // free(p2);
    free(p1);
    free(p3);
    return 0;
}