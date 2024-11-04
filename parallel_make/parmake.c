/**
 * parallel_make
 * CS 341 - Fall 2024
 */

#include <pthread.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include "set.h"
#include "vector.h"
#include "queue.h"
// rule_t->state = 1 -> rule is satisfied
// rule_t->state = 0 -> rule is not checked yet
// rule_t->state = -1 -> rule is failed

// Global variables
graph *g = NULL;
set *visited = NULL;
queue *q = NULL;
pthread_mutex_t m;
pthread_cond_t cv;
int failed = 0;


bool dependencies_satisfied(char *target) {
    vector *dependencies = graph_neighbors(g, target);
    bool satisfied = true;
    for (size_t i = 0; i < vector_size(dependencies); i++) {
        char *dependency = vector_get(dependencies, i);
        rule_t *dep_rule = (rule_t *)graph_get_vertex_value(g, dependency);
        if (dep_rule->state != 1) {
            satisfied = false;
            break;
        }
    }
    vector_destroy(dependencies);
    return satisfied;
}

// Function to check if dependencies are satisfied
bool dependencies_checked(char *target) {
    vector *dependencies = graph_neighbors(g, target);
    bool satisfied = true;
    for (size_t i = 0; i < vector_size(dependencies); i++) {
        char *dependency = vector_get(dependencies, i);
        rule_t *dep_rule = (rule_t *)graph_get_vertex_value(g, dependency);
        // printf("Dependency: %s\n", dependency);
        // printf("Dependency state: %d\n", dep_rule->state);
        if (dep_rule->state == 0) {
            satisfied = false;
            break;
        }
    }
    vector_destroy(dependencies);
    // printf("dependencies_satisfied return\n");
    return satisfied;
}

// Original cycle detection function provided by you.
bool has_cycle(graph* g, char* target) {
    if (!graph_contains_vertex(g, target)) {
        return false;
    }
    if (visited == NULL) {
        visited = shallow_set_create();
    }
    if (set_contains(visited, target)) {
        set_destroy(visited);
        visited = NULL;
        return true;
    }
    set_add(visited, target);
    vector* neighbors = graph_neighbors(g, target);
    for (size_t i = 0; i < vector_size(neighbors); i++) {
        char* neighbor = vector_get(neighbors, i);
        if (has_cycle(g, neighbor)) {
            vector_destroy(neighbors);
            set_destroy(visited);
            visited = NULL;
            return true;
        }
    }
    vector_destroy(neighbors);
    set_destroy(visited);
    visited = NULL;
    return false;
}

// Function to determine if a rule needs to be executed.
bool should_execute_rule(char* target) {

    // printf("Checking if rule should be executed for target: %s\n", target);
    vector* dependencies = graph_neighbors(g, target);
    bool target_exists_on_disk = (access(target, F_OK) == 0);
    bool flag_run = false;
   
    if (dependencies != NULL) { 
        for (size_t i = 0; i < vector_size(dependencies); i++) {
            char* dependency = vector_get(dependencies, i);
            // if (!set_contains(successful_builds, dependency)) {
            //     vector_destroy(dependencies);
            //     return false;
            // }
            rule_t* rule = (rule_t*)graph_get_vertex_value(g, dependency);
            if (rule->state == -1) {
                vector_destroy(dependencies);
                return false;
            }
        }
        // what if some dependencies are not checked yet? we need to wait for them to be checked.???
        // for (size_t i = 0; i < vector_size(dependencies); i++) {
        //     char* dependency = vector_get(dependencies, i);
        //     // if (!set_contains(successful_builds, dependency)) {
        //     //     vector_destroy(dependencies);
        //     //     return false;
        //     // }
        //     rule_t* rule = (rule_t*)graph_get_vertex_value(g, dependency);
        //     if (rule->state == 0) {
        //         vector_destroy(dependencies);
        //         return false;
        //     }
        // }
    } 
   
    // check if target exists on disk
    if (!target_exists_on_disk) {
        flag_run = true;
        // vector_destroy(dependencies);
        // return true;
    } else {
        bool has_dependencies_on_disk = false;
        for (size_t i = 0; i < vector_size(dependencies); i++) {
            char* dependency = vector_get(dependencies, i);
            if (access(dependency, F_OK) == 0) { // has dependency on disk
                has_dependencies_on_disk = true;
                break;
            }
        }
        if (!has_dependencies_on_disk) {
            flag_run = true;
            // vector_destroy(dependencies);
            // return true;
        } else {
            // check if any dependency has a newer modification time than the target
            for (size_t i = 0; i < vector_size(dependencies); i++) {
                char* dependency = vector_get(dependencies, i);
                struct stat target_stat;
                struct stat dep_stat;
                stat(target, &target_stat);
                stat(dependency, &dep_stat);
                time_t target_mod_time = target_stat.st_mtime;
                time_t dep_mod_time = dep_stat.st_mtime;
                if (difftime(dep_mod_time, target_mod_time) > 0) {
                    // printf("NHHHHHHHHHHHHH\n");
                    flag_run = true;
                    // vector_destroy(dependencies);
                    break;
                    // return true;
                } else {
                    rule_t* rule = (rule_t*)graph_get_vertex_value(g, target);
                    rule->state = 1; // rule satisfied
                }
            }

        }
    }

    rule_t *rule = (rule_t *)graph_get_vertex_value(g, target);
    for (size_t i = 0; i < vector_size(dependencies); i++) {
        char* dependency = vector_get(dependencies, i);
        rule_t *rule_nbr = (rule_t *) graph_get_vertex_value(g, dependency);
        
        if (rule_nbr->state == -1) {
            flag_run = false;
            rule->state = -1;
            // break;
            return false;
        }

        if (!(should_execute_rule(dependency)) && (rule_nbr->state == 0 || rule_nbr->state == -1)) {
            flag_run = false;
            rule_nbr->state = -1;
            rule->state = -1;
            // break;
            return false;
        }
    }
    
    if (vector_size(dependencies) != 0) {
        pthread_mutex_lock(&m);
        
        while (!dependencies_checked(target)) {
            // printf("waiting for dependencies to be satisfied\n");
            pthread_cond_wait(&cv, &m);
        }
        pthread_mutex_unlock(&m);
    }

    
    if (!dependencies_satisfied(target)) {
        // printf("Dependencies not satisfied\n");
        vector_destroy(dependencies);
        return false;
    }
    // printf("11111111111111111111111111111111111111\n");
    if (flag_run) {
        rule_t* rule = (rule_t*)graph_get_vertex_value(g, target);
        if (rule->state == 0 && dependencies_satisfied(target)) {
            // printf("pushing target '%s' to queue\n", target);
            queue_push(q, rule);
            vector_destroy(dependencies);
            return true;
        }
        if (rule->state == 1) {
            vector_destroy(dependencies);
            return false;
        }
        if (rule->state == -1) {
            vector_destroy(dependencies);
            return false;
        }
    }
    vector_destroy(dependencies);
    return false;
}


// Thread function to process rules from the queue
void *worker_thread(void *ptr) {
    while (1) {
        // printf("Thread %ld\n", pthread_self());
        // pthread_mutex_lock(&m);
        rule_t *rule = queue_pull(q);
    
        if (!rule) {
            // pthread_mutex_unlock(&m);
            queue_push(q, NULL); // Notify other threads to exit
            break;
        }
        // pthread_mutex_unlock(&m);

        // Execute commands for the rule
        for (size_t i = 0; i < vector_size(rule->commands); i++) {
            char *command = vector_get(rule->commands, i);
            // execute_commands(rule->target);
            if (system(command) != 0) {
                rule->state = -1; // Mark rule as failed
                // break;
                pthread_cond_signal(&cv);
                return NULL;
            }
        }

        // pthread_mutex_lock(&m);
        rule->state = 1; // Mark rule as completed
        pthread_cond_signal(&cv);
        // pthread_cond_broadcast(&cv); // Notify waiting threads
        // pthread_mutex_unlock(&m);
    }
    return NULL;
}

// Main function for parmake
int parmake(char *makefile, size_t num_threads, char **targets) {
    // Initialize synchronization primitives and queue
    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&cv, NULL);
    q = queue_create(-1);

    // Parse the Makefile and initialize the graph
    g = parser_parse_makefile(makefile, targets);
    vector *goals = graph_neighbors(g, "");

    // Create threads
    pthread_t threads[num_threads];
    for (size_t i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }

    // Check for cycles and add initial tasks to the queue
    for (size_t i = 0; i < vector_size(goals); i++) {
        char *goal = vector_get(goals, i);
        if (has_cycle(g, goal)) {
            print_cycle_failure(goal);
        } else {
            if (should_execute_rule(goal)) {
                rule_t *rule = (rule_t *)graph_get_vertex_value(g, goal);
                rule->state = 0; // Mark rule as not checked yet
            }
        }
    }

    // Wait for threads to complete
    queue_push(q, NULL); // Signal threads to terminate
    for (size_t i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Cleanup
    vector_destroy(goals);
    graph_destroy(g);
    queue_destroy(q);
    pthread_mutex_destroy(&m);
    pthread_cond_destroy(&cv);
    return 0;
}
