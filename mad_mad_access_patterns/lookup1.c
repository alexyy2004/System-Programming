/**
 * mad_mad_access_patterns
 * CS 341 - Fall 2024
 * [Group Working]
 * Group Member Netids: pjame2, boyangl3, yueyan2
 */
#include "tree.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses fseek() and fread() to access the data.

  ./lookup1 <data_file> <word> [<word> ...]
*/
int BinarySearch(FILE* file, char* word, int offset) {
  if (offset == 0) {
    return 0;
  }
  fseek(file, offset, SEEK_SET);
  BinaryTreeNode* node = malloc(sizeof(BinaryTreeNode));
  fread(node, sizeof(BinaryTreeNode), 1, file);
  int new_offset = offset + sizeof(BinaryTreeNode);
  fseek(file, new_offset, SEEK_SET);
  char cur_word[20];
  fread(cur_word, 20, 1, file);
  // char cur_word[10];
  // strcpy(cur_word, node->word);
  // printf("cur_word is: %s\n", cur_word);
  // printf("node word is: %s\n", node->word);
  // printf("word is: %s\n", word);
  // printf("node count is: %d\n", node->count);
  if (strcmp(cur_word, word) == 0) {
    printFound(cur_word, node->count, node->price);
    free(node);
    return 1;
  } else {
    if (strcmp(word, cur_word) < 0) {
      int result = BinarySearch(file, word, node->left_child);
      free(node);
      return result;
    } else {
      int result = BinarySearch(file, word, node->right_child);
      free(node);
      return result;
    }
  }
  free(node);
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    printArgumentUsage();
    exit(1);
  }

  FILE *file = fopen(argv[1], "r");
  if (file == NULL) {
    openFail(argv[1]);
    exit(2);
  }

  char root[4];
  fread(root, 1, 4, file);
  if (strcmp(root, "BTRE") != 0) {
    formatFail(argv[1]);
    exit(2);
  }
  // printf("root is: %s\n", root);

  for (int i = 2; i < argc; i++) {
    char* target = argv[i];
    int find = BinarySearch(file, target, 4);
    if (find == 0) {
      printNotFound(target);
    }
    // free(target);
  }
  fclose(file);
  return 0;
}
