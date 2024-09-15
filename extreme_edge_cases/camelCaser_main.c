/**
 * extreme_edge_cases
 * CS 341 - Fall 2024
 */
#include <stdio.h>
#include <stdlib.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"
#include "camelCaser_ref_utils.h"

int main() {
    // Feel free to add more test cases of your own!
    // if (test_camelCaser(&camel_caser, &destroy)) {
    //     printf("SUCCESS\n");
    // } else {
    //     printf("FAILED\n");
    // }
    // const char *string = "The Heisenbug is an incredible creature. Facenovel servers get their power from its indeterminism. Code smell can be ignored with INCREDIBLE use of air freshener. God objects are the new religion.";
    const char *string = "Hello, World ! . ";
    // char *test = "The Heisenbug is an incredible creature. Facenovel servers get their power from its indeterminism. Code smell can be ignored with INCREDIBLE use of air freshener. God objects are the new religion.";
   
    char **result = camel_caser(string);
    int i = 0;
    while (result[i] != NULL) {
        // puts(result[i]);
        i += 1;
    }
    destroy(result);
    // int result = test_camelCaser(camel_caser, destroy);
    // printf("%d", result);
}
