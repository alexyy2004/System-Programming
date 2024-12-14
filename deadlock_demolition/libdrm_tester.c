// /**
//  * deadlock_demolition
//  * CS 341 - Fall 2024
//  */
// #include "libdrm.h"

// #include <assert.h>
// #include <pthread.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// int main() {
//     drm_t *drm = drm_init();
//     // TODO your tests here
//     drm_destroy(drm);

//     return 0;
// }
// Test_Would_Deadlock.c

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "libdrm.h"

drm_t *drm1, *drm2;

void *thread_A(void *arg) {
    pthread_t tid = pthread_self();

    drm_wait(drm1, &tid);
    printf("Thread A (%lu) locked drm1.\n", tid);

    sleep(2); // Ensure Thread B proceeds to attempt lock

    drm_post(drm1, &tid);
    printf("Thread A (%lu) released drm1.\n", tid);

    return NULL;
}

void *thread_B(void *arg) {
    pthread_t tid = pthread_self();

    sleep(1); // Ensure Thread A locks drm1 first

    printf("Thread B (%lu) attempting to lock drm1...\n", tid);
    int result = drm_wait(drm1, &tid);
    if (result == 0) {
        printf("Thread B (%lu) lock on drm1 rejected to prevent deadlock.\n", tid);
    } else {
        printf("Thread B (%lu) locked drm1.\n", tid);
        drm_post(drm1, &tid);
        printf("Thread B (%lu) released drm1.\n", tid);
    }

    return NULL;
}

int main() {
    pthread_t t1, t2;

    drm1 = drm_init();

    pthread_create(&t1, NULL, thread_A, NULL);
    pthread_create(&t2, NULL, thread_B, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    drm_destroy(drm1);

    printf("Test Would Deadlock completed.\n");

    return 0;
}
