/**
 * extreme_edge_cases
 * CS 341 - Fall 2024
 */
#include <stdio.h>

#include "camelCaser_ref_utils.h"

int main() {
    // Enter the string you want to test with the reference here.
    char *input = "Hello, World ! . ";
    // This function prints the reference implementation output on the terminal.
    print_camelCaser(input);
    char *output[] = {"hello", "world", "", NULL};
    int result = check_output(input, output);
    printf("result is %d", result);

    // Put your expected output for the given input above.
    // char *correct[] = {"hello", NULL};
    char *correct[] = {"theHeisenbugIsAnIncredibleCreature",
                    "facenovelServersGetTheirPowerFromItsIndeterminism",
                    "codeSmellCanBeIgnoredWithIncredibleUseOfAirFreshener",
                    "godObjectsAreTheNewReligion",
                    NULL};
    // char *wrong[] = {"hello", "welcomeToCs241", NULL};

    // Compares the expected output you supplied with the reference output.
    printf("check_output test 1: %d\n", check_output(input, correct));
    // printf("check_output test 2: %d\n", check_output(input, wrong));

    // Feel free to add more test cases.
    return 0;
}
