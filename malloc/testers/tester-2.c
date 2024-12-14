/**
 * malloc
 * CS 341 - Fall 2024
 */
#include "tester-utils.h"

#define TOTAL_ALLOCS 5 * M

int main(int argc, char *argv[]) {
    malloc(1);

    int i;
    int **arr = malloc(TOTAL_ALLOCS * sizeof(int *));
    if (arr == NULL) {
        fprintf(stderr, "Memory failed to allocate!\n");
        return 1;
    }
    printf("finish the first malloc\n");

    for (i = 0; i < TOTAL_ALLOCS; i++) {
        arr[i] = malloc(sizeof(int));
        if (arr[i] == NULL) {
            fprintf(stderr, "Memory failed to allocate!\n");
            return 1;
        }

        *(arr[i]) = i;
    }
    printf("finish the first for loop\n");

    for (i = 0; i < TOTAL_ALLOCS; i++) {
        if (*(arr[i]) != i) {
            fprintf(stderr, "Memory failed to contain correct data after many "
                            "allocations!\n");
            return 2;
        }
    }
    printf("finish the second for loop\n");

    for (i = 0; i < TOTAL_ALLOCS; i++)
        free(arr[i]);
    printf("finish the free for loop\n");

    free(arr);
    fprintf(stderr, "Memory was allocated, used, and freed!\n");
    return 0;
}
