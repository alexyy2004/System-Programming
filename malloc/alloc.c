// /**
//  * malloc
//  * CS 341 - Fall 2024
//  */
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <stdint.h> // For SIZE_MAX

// /* I use Chatgpt for debugging and initial idea*/
// /**
//  * Metadata for each memory block.
//  */
// typedef struct block_meta_t {
//   size_t size;
//   void *ptr;
//   struct block_meta_t *next;
//   struct block_meta_t *prev;
//   int free;
// } block_meta_t;

// #define META_SIZE sizeof(block_meta_t)

// static block_meta_t *global_head = NULL;
// static size_t total_memory_requested = 0;
// static size_t total_memory_sbrk = 0;

// /**
//  * Aligns size to the nearest multiple of 8 bytes.
//  */
// size_t align8(size_t size) {
//   if (size & 0x7) {
//     size = (size & ~0x7) + 8;
//   }
//   return size;
// }

// /**
//  * Splits a block into two if it's larger than needed.
//  */
// int split_block(block_meta_t *block, size_t size) {
//   // if (block != NULL) {
//   //     printf("block original size: %zu\n", block->size);
//   // } else {
//   //     printf("block is NULL\n");
//   // }

//   if (block->size >= 2 * size && block->size - size >= 64) {
//     // fprintf(stderr, "%zu\n", block->size-size);
//     // if (block->size - size == 6) {
//     //   fprintf(stderr, "HERE IT IS!!!!\n");
//     // }
//     block_meta_t *new_block = (block_meta_t *)((char*)block->ptr + size);
//     new_block->ptr = new_block + 1;
//     new_block->size = block->size - size - META_SIZE;
//     // new_block->next = block->next;
//     // new_block->prev = block;
//     new_block->next = block;
//     new_block->free = 1;

//     if (block->prev) {
//       block->prev->next = new_block;
//     } else {
//       global_head = new_block;
//     }

//     new_block->prev = block->prev;
//     block->size = size;
//     block->prev = new_block;
//     // if (new_block->next) {
//     //     new_block->next->prev = new_block;
//     // }
//     // total_memory_requested += META_SIZE;
//     return 1;
//   }
//   return 0;
// }

// /**
//  * Checks for addition overflow in size calculations.
//  */
// int check_add_overflow(size_t a, size_t b) {
//   return a > SIZE_MAX - b;
// }

// /**
//  * Checks for multiplication overflow in size calculations.
//  */
// int check_mul_overflow(size_t a, size_t b) {
//   if (a == 0 || b == 0) return 0;
//   return a > SIZE_MAX / b;
// }

// /**
//  * Finds a free block with enough space.
//  */
// block_meta_t *find_free_block(size_t size) {
//   // printf("entered find_free_block\n");
//   block_meta_t *current = global_head;
//   // printf("global_head: %p\n", (void *)global_head);

//   // block_meta_t *temp = global_head;
//   // while (temp) {
//   //     printf("Block at %p: size=%zu, free=%d, next=%p, prev=%p\n",
//   //         (void *)temp, temp->size, temp->free, (void *)temp->next, (void *)temp->prev);
//   //     temp = temp->next;
//   // }

//   while (current) {
//     if (current->free && current->size >= size) {
//       if (split_block(current, size)) {
//         total_memory_requested += META_SIZE;
//       }
//       return current;
//     }
//     current = current->next;
//   }
//   return NULL;
// }

// /**
//  * Extends the heap by requesting more memory from the OS.
//  */
// block_meta_t *request_space(size_t size) {
//   // block_meta_t *block = NULL;
//   // if (check_add_overflow(size, META_SIZE)) {
//   //   return NULL;
//   // }
//   // size_t total_size = size + META_SIZE;
//   // void *sbrk_result = sbrk(total_size);
//   // if (sbrk_result == (void*) -1) {
//   //   // sbrk failed.
//   //   return NULL;
//   // }
//   // block = sbrk_result;
//   // // if (last) { // NULL on first request.
//   // //     last->next = block;
//   // //     block->prev = last;
//   // // } else {
//   // //     block->prev = NULL;
//   // // }

//   // block->size = size;
//   // block->ptr = block + 1;
//   // block->next = NULL;
//   // block->free = 0;
//   // return block;
//   block_meta_t* block = sbrk(size + META_SIZE);
//   if (block == (void *) - 1) {
//     return NULL;
//   }
//   block->ptr = block + 1;
//   block->size = size;
//   block->free = 0;
//   block->next = global_head;
//   block->prev = NULL;
//   if (global_head) {
//     global_head->prev = block;
//   }

//   global_head = block;
//   total_memory_sbrk += size + META_SIZE;
//   total_memory_requested += size + META_SIZE;
//   return block;
// }


// void coalescePrevOnly(block_meta_t *block) {
//   if (block->prev && block->prev->free) {
//     if (check_add_overflow(block->prev->size, block->size + META_SIZE)) {
//       return;
//     }
//     block->size += block->prev->size + META_SIZE;
//     block->prev = block->prev->prev;
//     if (block->prev) {
//       block->prev->next = block;
//     } else {
//       global_head = block;
//     }
//     // block = block->prev;
//     total_memory_requested -= META_SIZE;
//   }
// }

// /**
//  * Coalesces adjacent free blocks.
//  */
// void coalesce(block_meta_t *block) {
//   // Coalesce with previous block if it's free.
//   if (block->prev && block->prev->free) {
//     if (check_add_overflow(block->prev->size, block->size + META_SIZE)) {
//       return;
//     }
//     block->size += block->prev->size + META_SIZE;
//     block->prev = block->prev->prev;
//     if (block->prev) {
//       block->prev->next = block;
//     } else {
//       global_head = block;
//     }
//     // block = block->prev;
//     total_memory_requested -= META_SIZE;
//   }

//   // Coalesce with next block if it's free.
//   if (block->next && block->next->free) {
//     if (check_add_overflow(block->size, block->next->size + META_SIZE)) {
//       return;
//     }
//     // block->size += block->next->size + META_SIZE;
//     // block->next = block->next->next;
//     block->next->size += block->size + META_SIZE;
//     block->next->prev = block->prev;
//     if (block->prev) {
//       block->prev->next = block->next;
//     } else {
//       global_head = block->next;
//     }
//     total_memory_requested -= META_SIZE;
//   }

//   // // Coalesce with next block if it's free.
//   // if (block->next && block->next->free) {
//   //   if (check_add_overflow(block->size, block->next->size + META_SIZE)) {
//   //     return;
//   //   }
//   //   // block->size += block->next->size + META_SIZE;
//   //   // block->next = block->next->next;
//   //   block->next->size += block->size + META_SIZE;
//   //   block->next->prev = block->prev;
//   //   if (block->prev) {
//   //     block->prev->next = block->next;
//   //   } else {
//   //     global_head = block->next;
//   //   }
//   //   total_memory_requested -= META_SIZE;
//   // }
// }

// /**
//  * Allocate memory block
//  *
//  * Allocates a block of size bytes of memory, returning a pointer to the
//  * beginning of the block.  The content of the newly allocated block of
//  * memory is not initialized, remaining with indeterminate values.
//  *
//  * @param size
//  *    Size of the memory block, in bytes.
//  *
//  * @return
//  *    On success, a pointer to the memory block allocated by the function.
//  *
//  *    The type of this pointer is always void*, which can be cast to the
//  *    desired type of data pointer in order to be dereferenceable.
//  *
//  *    If the function failed to allocate the requested block of memory,
//  *    a null pointer is returned.
//  *
//  * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
//  */
// void *malloc(size_t size) {

//   if (size <= 0) {
//     return NULL;
//   }
//   block_meta_t *block = NULL;
//   if (total_memory_sbrk - total_memory_requested >= size) {
//     block = find_free_block(size);
//   }

//   // block = find_free_block(size);
//   if (block) {
//     // block = split_block(block, size);
//     // total_memory_requested += META_SIZE;
//     // printf("block size: %zu\n", block->size);
//     total_memory_requested += size;
//     block->free = 0;
//   } else {
//     if (global_head && global_head->free) {
//       if (sbrk(size - global_head->size) == (void *) - 1) {
//         return NULL;
//       }
//       total_memory_sbrk += size - global_head->size;
//       global_head->size = size;
//       global_head->free = 0;
//       block = global_head;
//       total_memory_requested += size;
//     } else {
//       // block = sbrk(size + META_SIZE);
//       // if (block == (void *) - 1) {
//       //     return NULL;
//       // }
//       // block->ptr = block + 1;
//       // block->size = size;
//       // block->free = 0;
//       // block->next = global_head;
//       // block->prev = NULL;
//       // if (global_head) {
//       //     global_head->prev = block;
//       // }

//       // global_head = block;
//       // total_memory_sbrk += size + META_SIZE;
//       // total_memory_requested += size + META_SIZE;
//       block = request_space(size);
//     }
//   }
//   return block->ptr;
// }

// /**
// * Deallocate space in memory
// *
// * A block of memory previously allocated using a call to malloc(),
// * calloc() or realloc() is deallocated, making it available again for
// * further allocations.
// *
// * Notice that this function leaves the value of ptr unchanged, hence
// * it still points to the same (now invalid) location, and not to the
// * null pointer.
// *
// * @param ptr
// *    Pointer to a memory block previously allocated with malloc(),
// *    calloc() or realloc() to be deallocated.  If a null pointer is
// *    passed as argument, no action occurs.
// */
// void free(void *ptr) {
//   if (!ptr) {
//     return;
//   }

//   block_meta_t *block = (block_meta_t *)ptr - 1;
//   block->free = 1;
//   total_memory_requested -= block->size;
//   // Coalesce adjacent free blocks.
//   coalesce(block);
//   return;
// }

// /**
//  * Reallocate memory block
//  *
//  * The size of the memory block pointed to by the ptr parameter is changed
//  * to the size bytes, expanding or reducing the amount of memory available
//  * in the block.
//  *
//  * The function may move the memory block to a new location, in which case
//  * the new location is returned. The content of the memory block is preserved
//  * up to the lesser of the new and old sizes, even if the block is moved. If
//  * the new size is larger, the value of the newly allocated portion is
//  * indeterminate.
//  *
//  * In case that ptr is NULL, the function behaves exactly as malloc, assigning
//  * a new block of size bytes and returning a pointer to the beginning of it.
//  *
//  * In case that the size is 0, the memory previously allocated in ptr is
//  * deallocated as if a call to free was made, and a NULL pointer is returned.
//  *
//  * @param ptr
//  *    Pointer to a memory block previously allocated with malloc(), calloc()
//  *    or realloc() to be reallocated.
//  *
//  *    If this is NULL, a new block is allocated and a pointer to it is
//  *    returned by the function.
//  *
//  * @param size
//  *    New size for the memory block, in bytes.
//  *
//  *    If it is 0 and ptr points to an existing block of memory, the memory
//  *    block pointed by ptr is deallocated and a NULL pointer is returned.
//  *
//  * @return
//  *    A pointer to the reallocated memory block, which may be either the
//  *    same as the ptr argument or a new location.
//  *
//  *    The type of this pointer is void*, which can be cast to the desired
//  *    type of data pointer in order to be dereferenceable.
//  *
//  *    If the function failed to allocate the requested block of memory,
//  *    a NULL pointer is returned, and the memory block pointed to by
//  *    argument ptr is left unchanged.
//  *
//  * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
//  */

// void *realloc(void *ptr, size_t size) {
//   if (!ptr) {
//     return malloc(size);
//   }

//   if (size == 0) {
//     free(ptr);
//     // return NULL;
//   }

//   block_meta_t *block = (block_meta_t *)ptr - 1;

//   if (block->size >= 2 * size) {
//     split_block(block, size);
//     total_memory_requested -= block->prev->size;
//   }

//   if (block->size >= size) {
//     return ptr;
//   } else if (block->prev && block->prev->free && block->size + block->prev->size + META_SIZE >= size) {
//     total_memory_requested += block->prev->size;
//     // coalesce(block);
//     // coalescePrevOnly(block);
//     return block->ptr;
//     // if (block->size >= size) {
//     //     split_block(block, size);
//     //     return ptr;
//     // }
//     // return ptr;
//   }

//   // Allocate new block.
//   void *new_block = malloc(size);
//   if (!new_block) {
//     return NULL;
//   }
//   // Copy data to new block.
//   // size_t copy_size = (old_size < size) ? old_size : size;
//   memcpy(new_block, ptr, block->size);
//   // Free old block.
//   free(ptr);
//   return new_block;
// }

// /**
//  * Allocate space for array in memory
//  *
//  * Allocates a block of memory for an array of num elements, each of them size
//  * bytes long, and initializes all its bits to zero. The effective result is
//  * the allocation of an zero-initialized memory block of (num * size) bytes.
//  *
//  * @param num
//  *    Number of elements to be allocated.
//  * @param size
//  *    Size of elements.
//  *
//  * @return
//  *    A pointer to the memory block allocated by the function.
//  *
//  *    The type of this pointer is always void*, which can be cast to the
//  *    desired type of data pointer in order to be dereferenceable.
//  *
//  *    If the function failed to allocate the requested block of memory, a
//  *    NULL pointer is returned.
//  *
//  * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
//  */
// void *calloc(size_t num, size_t size) {
//   if (check_mul_overflow(num, size)) {
//     return NULL;
//   }
  
//   void *ptr = malloc(num * size);
//   if (!ptr) {
//     return NULL;
//   }
//   // Initialize memory to zero.
//   memset(ptr, 0, num * size);
//   return ptr;
// }


/**
 * malloc
 * CS 341 - Fall 2024
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h> // For SIZE_MAX

/* I use Chatgpt for debugging and initial idea*/
/**
 * Metadata for each memory block.
 */
typedef struct block_meta_t {
  size_t size;
  void *ptr;
  struct block_meta_t *next;
  struct block_meta_t *prev;
  int free;
} block_meta_t;

#define META_SIZE sizeof(block_meta_t)

static block_meta_t *global_head = NULL;
static size_t total_memory_requested = 0;
static size_t total_memory_sbrk = 0;

/**
 * Aligns size to the nearest multiple of 8 bytes.
 */
size_t align8(size_t size) {
  if (size & 0x7) {
    size = (size & ~0x7) + 8;
  }
  return size;
}

/**
 * Splits a block into two if it's larger than needed.
 */
int split_block(block_meta_t *block, size_t size) {
  // if (block != NULL) {
  //     printf("block original size: %zu\n", block->size);
  // } else {
  //     printf("block is NULL\n");
  // }
  // block->size >= 2 * size && block->size - size >= 64
  if (block->size >= size + META_SIZE) {
    // fprintf(stderr, "%zu\n", block->size-size);
    // if (block->size - size == 6) {
    //   fprintf(stderr, "HERE IT IS!!!!\n");
    // }
    block_meta_t *new_block = (block_meta_t *)((char*)block->ptr + size);
    new_block->ptr = new_block + 1;
    new_block->size = block->size - size - META_SIZE;
    // new_block->next = block->next;
    // new_block->prev = block;
    new_block->next = block;
    new_block->free = 1;

    if (block->prev) {
      block->prev->next = new_block;
    } else {
      global_head = new_block;
    }

    new_block->prev = block->prev;
    block->size = size;
    block->prev = new_block;
    // if (new_block->next) {
    //     new_block->next->prev = new_block;
    // }
    // total_memory_requested += META_SIZE;
    return 1;
  }
  return 0;
}

/**
 * Checks for addition overflow in size calculations.
 */
int check_add_overflow(size_t a, size_t b) {
  return a > SIZE_MAX - b;
}

/**
 * Checks for multiplication overflow in size calculations.
 */
int check_mul_overflow(size_t a, size_t b) {
  if (a == 0 || b == 0) return 0;
  return a > SIZE_MAX / b;
}

/**
 * Finds a free block with enough space.
 */
block_meta_t *find_free_block(size_t size) {
  // printf("entered find_free_block\n");
  block_meta_t *current = global_head;
  // printf("global_head: %p\n", (void *)global_head);

  // block_meta_t *temp = global_head;
  // while (temp) {
  //     printf("Block at %p: size=%zu, free=%d, next=%p, prev=%p\n",
  //         (void *)temp, temp->size, temp->free, (void *)temp->next, (void *)temp->prev);
  //     temp = temp->next;
  // }

  while (current) {
    if (current->free && current->size >= size) {
      if (split_block(current, size)) {
        total_memory_requested += META_SIZE;
      }
      return current;
    }
    current = current->next;
  }
  return NULL;
}

/**
 * Extends the heap by requesting more memory from the OS.
 */
block_meta_t *request_space(size_t size) {
  // block_meta_t *block = NULL;
  // if (check_add_overflow(size, META_SIZE)) {
  //   return NULL;
  // }
  // size_t total_size = size + META_SIZE;
  // void *sbrk_result = sbrk(total_size);
  // if (sbrk_result == (void*) -1) {
  //   // sbrk failed.
  //   return NULL;
  // }
  // block = sbrk_result;
  // // if (last) { // NULL on first request.
  // //     last->next = block;
  // //     block->prev = last;
  // // } else {
  // //     block->prev = NULL;
  // // }

  // block->size = size;
  // block->ptr = block + 1;
  // block->next = NULL;
  // block->free = 0;
  // return block;
  block_meta_t* block = sbrk(size + META_SIZE);
  if (block == (void *) - 1) {
    return NULL;
  }
  block->ptr = block + 1;
  block->size = size;
  block->free = 0;
  block->next = global_head;
  block->prev = NULL;
  if (global_head) {
    global_head->prev = block;
  }

  global_head = block;
  total_memory_sbrk += size + META_SIZE;
  total_memory_requested += size + META_SIZE;
  return block;
}


void coalescePrevOnly(block_meta_t *block) {
  if (block->prev && block->prev->free) {
    if (check_add_overflow(block->prev->size, block->size + META_SIZE)) {
      return;
    }
    block->size += block->prev->size + META_SIZE;
    block->prev = block->prev->prev;
    if (block->prev) {
      block->prev->next = block;
    } else {
      global_head = block;
    }
    // block = block->prev;
    total_memory_requested -= META_SIZE;
  }
}

/**
 * Coalesces adjacent free blocks.
 */
void coalesce(block_meta_t *block) {
  // Coalesce with previous block if it's free.
  if (block->prev && block->prev->free) {
    if (check_add_overflow(block->prev->size, block->size + META_SIZE)) {
      return;
    }
    block->size += block->prev->size + META_SIZE;
    block->prev = block->prev->prev;
    if (block->prev) {
      block->prev->next = block;
    } else {
      global_head = block;
    }
    // block = block->prev;
    total_memory_requested -= META_SIZE;
  }

  // Coalesce with next block if it's free.
  if (block->next && block->next->free) {
    if (check_add_overflow(block->size, block->next->size + META_SIZE)) {
      return;
    }
    // block->size += block->next->size + META_SIZE;
    // block->next = block->next->next;
    block->next->size += block->size + META_SIZE;
    block->next->prev = block->prev;
    if (block->prev) {
      block->prev->next = block->next;
    } else {
      global_head = block->next;
    }
    total_memory_requested -= META_SIZE;
  }
}

/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {

  if (size <= 0) {
    return NULL;
  }
  block_meta_t *block = NULL;
  if (total_memory_sbrk - total_memory_requested >= size) {
    block = find_free_block(size);
  }

  // block = find_free_block(size);
  if (block) {
    // block = split_block(block, size);
    // total_memory_requested += META_SIZE;
    // printf("block size: %zu\n", block->size);
    total_memory_requested += size;
    block->free = 0;
  } else {
    if (global_head && global_head->free) {
      if (sbrk(size - global_head->size) == (void *) - 1) {
        return NULL;
      }
      total_memory_sbrk += size - global_head->size;
      global_head->size = size;
      global_head->free = 0;
      block = global_head;
      total_memory_requested += size;
    } else {
      // block = sbrk(size + META_SIZE);
      // if (block == (void *) - 1) {
      //     return NULL;
      // }
      // block->ptr = block + 1;
      // block->size = size;
      // block->free = 0;
      // block->next = global_head;
      // block->prev = NULL;
      // if (global_head) {
      //     global_head->prev = block;
      // }

      // global_head = block;
      // total_memory_sbrk += size + META_SIZE;
      // total_memory_requested += size + META_SIZE;
      block = request_space(size);
    }
  }
  return block->ptr;
}

/**
* Deallocate space in memory
*
* A block of memory previously allocated using a call to malloc(),
* calloc() or realloc() is deallocated, making it available again for
* further allocations.
*
* Notice that this function leaves the value of ptr unchanged, hence
* it still points to the same (now invalid) location, and not to the
* null pointer.
*
* @param ptr
*    Pointer to a memory block previously allocated with malloc(),
*    calloc() or realloc() to be deallocated.  If a null pointer is
*    passed as argument, no action occurs.
*/
void free(void *ptr) {
  if (!ptr) {
    return;
  }

  block_meta_t *block = (block_meta_t *)ptr - 1;
  block->free = 1;
  total_memory_requested -= block->size;
  // Coalesce adjacent free blocks.
  coalesce(block);
  return;
}

/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */

void *realloc(void *ptr, size_t size) {
  if (!ptr) {
    return malloc(size);
  }

  if (size == 0) {
    free(ptr);
    // return NULL;
  }

  block_meta_t *block = (block_meta_t *)ptr - 1;

  if (block->size >= 2 * size) {
    split_block(block, size);
    total_memory_requested -= block->prev->size;
  }

  if (block->size >= size) {
    return ptr;
  } else if (block->prev && block->prev->free && block->size + block->prev->size + META_SIZE >= size) {
    total_memory_requested += block->prev->size;
    // coalesce(block);
    coalescePrevOnly(block);
    return block->ptr;
    // if (block->size >= size) {
    //     split_block(block, size);
    //     return ptr;
    // }
    // return ptr;
  }

  // Allocate new block.
  void *new_block = malloc(size);
  if (!new_block) {
    return NULL;
  }
  // Copy data to new block.
  // size_t copy_size = (old_size < size) ? old_size : size;
  memcpy(new_block, ptr, block->size);
  // Free old block.
  free(ptr);
  return new_block;
}

/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
  if (check_mul_overflow(num, size)) {
    return NULL;
  }
  
  void *ptr = malloc(num * size);
  if (!ptr) {
    return NULL;
  }
  // Initialize memory to zero.
  memset(ptr, 0, num * size);
  return ptr;
}