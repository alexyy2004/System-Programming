/**
 * extreme_edge_cases
 * CS 341 - Fall 2024
 */
#include "camelCaser.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

char **split(const char *input_str) { //split according to "."
    char **result = NULL;
    int idx = 0;
    int prev_word_end_idx = 0;
    int number_of_strings = 0;
    size_t str_length = strlen(input_str);
    
    while ((unsigned int)idx < str_length) {
        if (ispunct(input_str[idx])) {
            char *temp = malloc(idx - prev_word_end_idx + 1);
            strncpy(temp, &input_str[prev_word_end_idx], idx - prev_word_end_idx);
            temp[idx - prev_word_end_idx] = '\0';
            prev_word_end_idx = idx + 1;
            number_of_strings += 1;

            size_t size = number_of_strings * sizeof(char*);
            result = realloc(result, size);
            result[number_of_strings - 1] = temp;
        }
        idx += 1;
    }
    number_of_strings += 1;
    result = realloc(result, number_of_strings * sizeof(char*));
    result[number_of_strings - 1] = NULL;
    return result;
}

char **camel_caser(const char *input_str) {
    // TODO: Implement me!
    if (input_str == NULL) {
        return NULL;
    }
    char **split_str = split(input_str);
    int num_of_strings = 0;
    while (split_str[num_of_strings] != NULL) {
        num_of_strings += 1;
    }
    char **result = malloc((num_of_strings + 1) * sizeof(char*));

    int i = 0;
    int count = 0; // the index of char * in result
    while (split_str[i] != NULL) {
        bool flag = true; // true = start of a new word
        char *temp = split_str[i];
        char *new_str = malloc(strlen(temp) + 1);
        int new_idx = 0;
        for (int j = 0; (unsigned int)j < strlen(temp); j++) {
            if (!ispunct(temp[j]) && !isalpha(temp[j]) && !isspace(temp[j])) {
                new_str[new_idx] = temp[j];
                new_idx += 1;
            } else {
                if (isalpha(temp[j])) {
                    if (new_idx == 0) { // temp[j] is the first char in this sentence
                        new_str[new_idx] = tolower(temp[j]);
                        new_idx += 1;
                        flag = false;
                    } else {
                        if (flag) {
                            new_str[new_idx] = toupper(temp[j]);
                            new_idx += 1;
                            flag = false;
                        } else {
                            new_str[new_idx] = tolower(temp[j]);
                            new_idx += 1;
                        }
                    }
                }
                if (isspace(temp[j])) {
                    flag = true;
                }
            }
        }
        new_str[strlen(temp)] = '\0';
        result[count] = new_str;
        count += 1;
        i += 1;
    }
    result[num_of_strings] = NULL;
    destroy(split_str);
    return result;
}


void destroy(char **result) {
    // TODO: Implement me!
    char **temp = result;
    int i = 0;
    while (temp[i] != NULL) {
        char *cur = temp[i];
        free(cur);
        i += 1;
    }
    free(result);
    return;
}

