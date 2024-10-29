/**
 * parallel_make
 * CS 341 - Fall 2024
 */

#include <sys/stat.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include "set.h"
#include <sys/types.h>
#include "vector.h"
#include <unistd.h>
// rule_t->state = 1 -> rule is satisfied
// rule_t->state = 0 -> rule is not checked yet
// rule_t->state = -1 -> rule is failed

// Global variable for the dependency graph.
graph* g = NULL;
set* visited = NULL;

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

// Helper function to perform topological sorting.
void topological_sort(graph* g, char* target, set* visited, vector* sorted) {
    if (!graph_contains_vertex(g, target) || set_contains(visited, target)) {
        return;
    }

    // Mark the target as visited.
    set_add(visited, target);

    // Get the neighbors (dependencies) of the target.
    vector* neighbors = graph_neighbors(g, target);
    for (size_t i = 0; i < vector_size(neighbors); i++) {
        char* neighbor = vector_get(neighbors, i);
        // Recursively sort the neighbors.
        topological_sort(g, neighbor, visited, sorted);
    }

    // Add the target to the sorted vector (acts like a stack).
    vector_push_back(sorted, target);
    vector_destroy(neighbors);
}

// Function to determine if a rule needs to be executed.
bool should_execute_rule(char* target, set* successful_builds) {
    vector* dependencies = graph_neighbors(g, target);
    bool target_exists_on_disk = (access(target, F_OK) == 0);

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
        vector_destroy(dependencies);
        return true;
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
            vector_destroy(dependencies);
            return true;
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
                    vector_destroy(dependencies);
                    return true;
                }
            }
            rule_t* rule = (rule_t*)graph_get_vertex_value(g, target);
            rule->state = 1; // rule satisfied
        }
    }
    vector_destroy(dependencies);
    return false;
}

// Function to execute commands for a target.
void execute_commands(char* target, set* successful_builds) {
    // Get the rule associated with the target.
    rule_t* rule = (rule_t*)graph_get_vertex_value(g, target);

    // Check if the rule needs to be executed.
    if (!should_execute_rule(target, successful_builds)) {
        // printf("Target '%s' is up to date.\n", target);
        // vector_destroy(dependencies);
        return; // The rule is already satisfied or cannot proceed due to a failed dependency.
    }

    // Get the commands vector from the rule.
    vector* commands = rule->commands;
    if (!commands) {
        // vector_destroy(dependencies);
        return;
    }

    // printf("%ld\n", vector_size(commands));
    // Execute each command in the vector.
    for (size_t i = 0; i < vector_size(commands); i++) {
        char* command = vector_get(commands, i);
        // printf("%s\n", command);
        if (system(command) != 0) {
            // fprintf(stderr, "Error: Command failed for target '%s': %s\n", target, strerror(errno));
            rule->state = -1; // Mark this rule as failed.

            // Mark this rule as failed by not adding it to the successful_builds set.
            // vector_destroy(dependencies);
            return; // Stop further execution on failure.
        }
    }

    // If all commands succeed, add this target to the successful_builds set.
    set_add(successful_builds, target);
    rule->state = 1; // Mark this rule as satisfied.

    // Clean up resources.
    // vector_destroy(dependencies);
}

int parmake(char *makefile, size_t num_threads, char **targets) {
    // Parse the Makefile into a dependency graph.
    g = parser_parse_makefile(makefile, targets);
    if (!g) {
        return 1;
    }

    // Retrieve goal rules (direct descendants of the sentinel node).
    vector* goals = graph_neighbors(g, "");

    // Create a vector to hold the topologically sorted nodes.
    vector* sorted = string_vector_create();
    set* sort_visited = shallow_set_create();
    set* successful_builds = shallow_set_create();

    // Check for cycles and perform topological sorting if no cycles are found.
    for (size_t i = 0; i < vector_size(goals); i++) {
        char* goal = vector_get(goals, i);
        if (graph_contains_vertex(g, goal)) {
            if (has_cycle(g, goal)) {
                print_cycle_failure(goal);
            } else {
                topological_sort(g, goal, sort_visited, sorted);
            }
        }
    }

    // Execute commands for each rule in topologically sorted order.
    for (int i = 0; i < (int)vector_size(sorted); i++) {
        char* target = vector_get(sorted, i);
        // printf("Target: %s\n", target);
        execute_commands(target, successful_builds);
    }

    // Cleanup resources.
    vector_destroy(goals);
    set_destroy(sort_visited);
    set_destroy(successful_builds);
    vector_destroy(sorted);
    graph_destroy(g);

    return 0;
}