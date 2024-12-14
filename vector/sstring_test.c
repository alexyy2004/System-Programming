/**
 * vector
 * CS 341 - Fall 2024
 */
#include "sstring.h"

int main(int argc, char *argv[]) {
    // TODO create some tests
    sstring *str = cstr_to_sstring("hello world!");
    char *cstr = sstring_to_cstr(str);
    printf("%s\n", cstr);
    // free(cstr);
    // sstring_destroy(str);

    // vector *vec = sstring_split(cstr_to_sstring("abcdeefg"), 'e');
    vector *vec = sstring_split(cstr_to_sstring("This is a sentence."), ' ');
    
    for (size_t i = 0; i < vector_size(vec); i++) {
        char *c = (char *)vector_get(vec, i);
        printf("%s\n", c);
    }
    printf("%zu\n", vector_size(vec));
    
    vector_destroy(vec);
    return 0;
}
