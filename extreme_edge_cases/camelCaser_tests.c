/**
 * extreme_edge_cases
 * CS 341 - Fall 2024
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

int test(char *input, char **target, char **(*camelCaser)(const char *), void (*destroy)(char **)) {
    char **output = camelCaser(input);
    int idx = 0;
    while (target[idx] != NULL) {
        if (strcmp(target[idx], output[idx]) != 0) {
            destroy(output);
            return 0;
        }
        idx += 1;
    }
    destroy(output);
    return 1;
}


int test_camelCaser(char **(*camelCaser)(const char *),
                    void (*destroy)(char **)) {
    // TODO: Implement me!
    char *input_1 = "";
    if (!test(input_1, NULL, camelCaser, destroy)) {
        return 0;
    }

    char *input_2 = "AAA";
    if (!test(input_2, NULL, camelCaser, destroy)) {
        return 0;
    }

    char *input_3 = "!!!";
    if (!test(input_3, (char *[]){"", "", "", NULL}, camelCaser, destroy)) {
        return 0;
    }

    char *input_4 = "What is this! What is that?";
    if (!test(input_4, (char *[]){"whatIsThis", "whatIsThat", NULL}, camelCaser, destroy)) {
        return 0;
    }

    char *input_5 = "  Leading spaces!   ";
    if (!test(input_5, (char *[]){"leadingSpaces", NULL}, camelCaser, destroy)) {
        return 0;
    }

    char *input_6 = "123 is a number.";
    if (!test(input_6, (char *[]){"123IsANumber", NULL}, camelCaser, destroy)) {
        return 0;
    }

    char *input_7 = "Line1\nLine2\tLine3.";
    if (!test(input_7, (char *[]){"line1Line2Line3", NULL}, camelCaser, destroy)) {
        return 0;
    }

    char *input_8 = "Hello...World!!! What???";
    if (!test(input_8, (char *[]){"hello",
        "",
        "",
        "world",
        "",
        "",
        "what",
        "",
        "",
        NULL}, camelCaser, destroy)) {
        return 0;
    }

    char *input_9 = "    ";
    if (!test(input_9, NULL, camelCaser, destroy)) {
        return 0;
    }

    char *input_10 = "Hello, World ! . ";
    if (!test(input_10, (char *[]){"hello",
        "world",
        "",
        NULL}, camelCaser, destroy)) {
        return 0;
    }

    return 1;
}