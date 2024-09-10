/**
 * perilous_pointers
 * CS 341 - Fall 2024
 */
#include "part2-functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * (Edit this function to print out the "Illinois" lines in
 * part2-functions.c in order.)
 */
int main() {
    // your code here
    first_step(81);

    int v1 = 132;
    int *value_1 = &v1;
    second_step(value_1);

    int temp = 8942;
    int* ptr = &temp;
    int** value_2 = &ptr;
    double_step(value_2);

    char value_strange[7] = {0,0,0,0,0,0,0};
    int n = 15;
    value_strange[5] = (char) n;
    strange_step(value_strange);

    char value_empty[5] = {0, 0, 0, 0, 0};
    empty_step((void *)(value_empty));

    char *value_twostep = "uuuu";
    char *s = "uuuu";
    two_step((void*)(s), value_twostep);

    char* first_threestep = "abcdefgh";
    //printf("first + 2 : %s\n", first_threestep+2);
    char* second_threestep = first_threestep + 2;
    // printf("second : %s\n", second_threestep);
    // printf("second + 2: %s\n", second_threestep+2);
    char* third_threestep = second_threestep + 2;
    // printf("third : %s\n", third_threestep);
    three_step(first_threestep, second_threestep, third_threestep);

    
    char* first_stepstepstep = "abcdefghij";
    char second_stepstepstep[20];
    second_stepstepstep[2] = first_stepstepstep[1] + 8;
    char third_stepstepstep[20];
    third_stepstepstep[3] = second_stepstepstep[2] + 8;
    step_step_step(first_stepstepstep, second_stepstepstep, third_stepstepstep);

    int b = 1;
    char c = 1;
    char* a = &c;
    it_may_be_odd(a, b);

    char str[] = " ,CS341";
    tok_step(str);

    char blue[4] = {1,2,0,0};
    int* orange = (int*)blue;
    // printf("%d", *orange);
    the_end((void*)(orange), (void*)(blue));
    // char blue[10];
	// for (int i = 0; i < 10; i++) blue[i] = 0;
	// blue[0] = 1; blue[1] = 2; blue[2] = 0; blue[3] = 0;
	// the_end(blue, blue); 

    return 0;
}
