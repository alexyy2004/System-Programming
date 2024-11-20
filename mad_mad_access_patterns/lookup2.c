/**
 * mad_mad_access_patterns
 * CS 341 - Fall 2024
 */
#include "tree.h"
#include "utils.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>

/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses mmap to access the data.

  ./lookup2 <data_file> <word> [<word> ...]
*/

int BinarySearch(char *addr, char *word, int offset) {
  if (offset == 0) {
    return 0;
  }

  BinaryTreeNode *node = (BinaryTreeNode *)(addr + offset);
  char *cur_word = node->word;
  // printf("cur_word is: %s\n", cur_word);
  
  if (strcmp(cur_word, word) == 0) {
    printFound(cur_word, node->count, node->price);
    // free(node);
    return 1;
  } else {
    if (strcmp(word, cur_word) < 0) {
      int result = BinarySearch(addr, word, node->left_child);
      // free(node);
      return result;
    } else {
      int result = BinarySearch(addr, word, node->right_child);
      // free(node);
      return result;
    }
  }
  // free(node);
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
    exit(1);
  }

  struct stat sb;
  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    openFail(argv[1]);
    exit(1);
  }
  fstat(fd, &sb);

  char *addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  // printf("addr is: %c\n", *(addr+1));
  if (addr == MAP_FAILED) {
    mmapFail(argv[1]);
    exit(1);
  }

  if (strncmp(addr, "BTRE", 4) != 0) {
    formatFail(argv[1]);
    exit(1);
  }

  for (int i = 2; i < argc; i++) {
    // printf("i is: %d\n", i);
    char* target = argv[i];
    // printf("target is: %s\n", target);
    int find = BinarySearch(addr, target, 4);

    if (find == 0) {
      printNotFound(target);
    }
  }
  munmap(addr,sb.st_size);
  fclose(file);
  return 0; 
}