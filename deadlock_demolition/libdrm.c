/**
 * deadlock_demolition
 * CS 341 - Fall 2024
 * Worked with boyangl3, yueyan2, pjame2 on this lab
 */
#include "graph.h"
#include "libdrm.h"
#include "set.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct drm_t {
    pthread_mutex_t m;
} drm_t;



static graph * g = NULL;
static pthread_mutex_t mut = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;


// Helper function for cycle detection
bool detectCycleUtil(void *vertex, set *visited, set *recStack) {
    // Mark the current node as visited and part of recursion stack
    set_add(visited, vertex);
    set_add(recStack, vertex);

    // Get all neighbors (adjacent vertices) of the current vertex
    vector *neighbors = graph_neighbors(g, vertex);

    // Iterate over the neighbors
    for (size_t i = 0; i < vector_size(neighbors); i++) {
        void *neighbor = vector_get(neighbors, i);

        if (!set_contains(visited, neighbor)) {
            // Recur for unvisited neighbor
            if (detectCycleUtil(neighbor, visited, recStack)) {
                vector_destroy(neighbors);
                return true;
            }
        } else if (set_contains(recStack, neighbor)) {
            // If neighbor is in recursion stack, then we have a cycle
            vector_destroy(neighbors);
            return true;
        }
    }

    // Remove the vertex from recursion stack
    set_remove(recStack, vertex);

    // Clean up
    vector_destroy(neighbors);

    return false;
}

// Function to detect cycle in the graph starting from 'start' vertex
bool detectCycle(void *start) {
    // Create sets to keep track of visited vertices and the recursion stack
    set *visited = shallow_set_create();
    set *recStack = shallow_set_create();
    bool hasCycle = detectCycleUtil(start, visited, recStack);

    // Clean up
    set_destroy(visited);
    set_destroy(recStack);

    return hasCycle;
}

// static MAX_NODES = 0;
// bool detectCycle();
// bool detectCycleUtil(int vertex, vector * visited, vector * recStack);
// bool detectCycle(void *start);
// bool vector_contains(vector *v, void *element);

// bool vector_contains(vector *v, void *element) {
//     for (size_t idx = 0; idx < vector_size(v); idx++) {
//         if (vector_get(v, idx) == element) {
//             return true;
//         }
//     }
//     return false;
// }

// bool detectCycle(void *start) {
//     vector *stack = shallow_vector_create();
//     vector *visited = shallow_vector_create();
//     vector_push_back(stack, start);

//     while (vector_size(stack) != 0) {
//         void *current = vector_back(stack);
//         vector_pop_back(stack);
//         if (!vector_contains(visited, current)) {
//             vector_push_back(visited, current);
//             vector *neighs = graph_neighbors(g, current);
//             for (size_t idx = 0; idx < vector_size(neighs); idx++) {
//                 if (!vector_contains(visited, vector_get(neighs, idx))) {
//                     vector_push_back(stack, vector_get(neighs, idx));
//                 } else {
//                     return true;
//                 }
//             }
//         }
//     }
//     return false;
// }

drm_t *drm_init() {
    drm_t * d = malloc(sizeof(drm_t));
    d->m = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&mut);
    if (g == NULL) {
        g = shallow_graph_create();
    }
    graph_add_vertex(g, d);
    pthread_mutex_unlock(&mut);
    return d;
}

int drm_post(drm_t *drm, pthread_t *thread_id) {
    pthread_mutex_lock(&mut);

    // check if drm and thread_id are in the graph
    if (!graph_contains_vertex(g, drm) || !graph_contains_vertex(g, thread_id)) {
        pthread_mutex_unlock(&mut);
        return 0;
    }

    // both must exist in the graph
    if (graph_adjacent(g, drm, thread_id)) {
        graph_remove_edge(g, drm, thread_id);
        pthread_mutex_unlock(&(drm->m));
        pthread_mutex_unlock(&mut);
        return 1;
    }

    pthread_mutex_unlock(&mut);
    return 0;
}

int drm_wait(drm_t *drm, pthread_t *thread_id) {
    pthread_mutex_lock(&mut);

    if (!graph_contains_vertex(g, drm)) {
        graph_add_vertex(g, drm);
    }

    if (!graph_contains_vertex(g, thread_id)) {
        graph_add_vertex(g, thread_id);
    }

    // this thread already own this mutex
    if (graph_adjacent(g, drm, thread_id)) {
        pthread_mutex_unlock(&mut);
        return 0;
    }

    graph_add_edge(g, thread_id, drm);
    if (detectCycle(thread_id)) {
        graph_remove_edge(g, thread_id, drm);
        pthread_mutex_unlock(&mut);
        return 0;
    }

    // this will not cause deadlock
    pthread_mutex_unlock(&mut); // let other thread work
    pthread_mutex_lock(&(drm->m));
    pthread_mutex_lock(&mut); 

    graph_add_edge(g, drm, thread_id);
    pthread_mutex_unlock(&mut);
    return 1;
}

void drm_destroy(drm_t *drm) {
    pthread_mutex_lock(&mut);

    // remove from graph
    if (graph_contains_vertex(g, drm)) {
        graph_remove_vertex(g, drm);
    }

    pthread_mutex_destroy(&(drm->m));
    free(drm);
    pthread_mutex_unlock(&mut);
    return;
}
