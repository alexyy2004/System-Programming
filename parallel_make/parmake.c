// /**
//  * parallel_make
//  * CS 341 - Fall 2024
//  */

#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include "set.h"
#include "vector.h"
#include <stdio.h>

// Global variable for the dependency graph.
graph* g = NULL;
set* visited = NULL;

void execute_commands(char* target) {
    // Get the rule associated with the target.
    rule_t* rule = (rule_t*)graph_get_vertex_value(g, target);
    if (!rule) {
        return;
    }

    // Get the commands vector from the rule.
    vector* commands = rule->commands;
    if (!commands) {
        return;
    }
    // syscall each command in the vector.
    for (size_t i = 0; i < vector_size(commands); i++) {
        if (system(vector_get(commands, i)) != 0) {
            break;
        }
    }
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
        printf("Target: %s\n", target);
        execute_commands(target);
    }
    printf("size of sorted: %ld\n", vector_size(sorted));



    // Cleanup resources.
    vector_destroy(goals);
    set_destroy(sort_visited);
    vector_destroy(sorted);
    graph_destroy(g);

    return 0;
}
