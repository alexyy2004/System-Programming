/**
 * malloc
 * CS 341 - Fall 2024
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h> // For SIZE_MAX

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

// int splitBlock(size_t size, block_meta_t *entry) {
//   if (entry->size >= 2*size && (entry->size-size) >= 1024) {
//     // printf("split block\n");
//     block_meta_t *new_entry = entry->ptr + size;
//     new_entry->ptr = (new_entry + 1);
//     // printf("splitBlock segfault end\n");
//     new_entry->free = 1;
//     new_entry->size = entry->size - size - sizeof(block_meta_t);
//     new_entry->next = entry;
//     if (entry->prev) {
//       entry->prev->next = new_entry;
//     } else {
//       global_head = new_entry;
//     }
//     new_entry->prev = entry->prev;
//     entry->size = size;
//     entry->prev = new_entry;
//     // printf("entry->size: %zu\n", entry->size);
//     // printf("new_entry->size: %zu\n", new_entry->size);
//     // block_meta_t *temp = head;
//     // while(temp){
//     //   printf("block->size: %zu, %d\n", temp->size, temp->free);
//     //   temp = temp->next;
//     // }
//     return 1;
//   }
//   return 0;
// }

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
    block_meta_t *current = global_head;
    while (current) {
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/**
 * Extends the heap by requesting more memory from the OS.
 */
// block_meta_t *request_space(size_t size) {
//     block_meta_t *block = NULL;
//     if (check_add_overflow(size, META_SIZE)) {
//         return NULL;
//     }
//     size_t total_size = size + META_SIZE;
//     void *sbrk_result = sbrk(total_size);
//     if (sbrk_result == (void*) -1) {
//         // sbrk failed.
//         return NULL;
//     }
//     block = sbrk_result;
//     // if (last) { // NULL on first request.
//     //     last->next = block;
//     //     block->prev = last;
//     // } else {
//     //     block->prev = NULL;
//     // }

//     block->size = size;
//     block->ptr = block + 1;
//     block->next = NULL;
//     block->free = 0;
//     return block;
// }

/**
 * Splits a block into two if it's larger than needed.
 */
block_meta_t* split_block(block_meta_t *block, size_t size) {
    if (block->size >= 2 * size) {
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
    }
    return block;
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
    block_meta_t *block = NULL;

    if (size <= 0) {
        return NULL;
    }

    if (total_memory_sbrk - total_memory_requested >= size) {
        block = find_free_block(size);
    }
    // block = find_free_block(size);
    if (block) {
        block = split_block(block, size);
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
            block = sbrk(size + META_SIZE);
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
}


void coalescePrev(block_meta_t *p) {
    p->size += p->prev->size+sizeof(block_meta_t);
    p->prev = p->prev->prev;
    if (p->prev) {
      p->prev->next = p;
    }
    else {
      global_head = p;
    }
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
        block = split_block(block, size);
        total_memory_requested -= block->prev->size;
    }

    if (block->size >= size) {
        return ptr;
    } else if (block->prev && block->prev->free && block->size + block->prev->size + sizeof(block_meta_t) >= size) {
        total_memory_requested += block->prev->size;
        // coalesce(block);
        coalescePrev(block);
        return block->ptr;
            // if (block->size >= size) {
            //     split_block(block, size);
            //     return ptr;
            // }
            // return ptr;
    }

        // Allocate new block.
    void *new_ptr = malloc(size);
    if (!new_ptr) {
        return NULL;
    }
    // Copy data to new block.
    // size_t copy_size = (old_size < size) ? old_size : size;
    memcpy(new_ptr, ptr, block->size);
    // Free old block.
    free(ptr);
    return new_ptr;
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
    size_t total_size = num * size;
    void *ptr = malloc(total_size);
    if (!ptr) {
        return NULL;
    }
    // Initialize memory to zero.
    memset(ptr, 0, total_size);
    return ptr;
}


// void coalesceBlock(block_meta_t *p) {
//   if (p->prev && p->prev->free == 1) {
//     // printf("p->size: %zu\n", p->size);
//     // printf("p->next->size: %zu\n", p->next->size);
//     // printf("only coalesce prev\n");
//     coalescePrev(p);
//     total_memory_requested -= sizeof(block_meta_t);
//   }
//   if (p->next && p->next->free == 1) {
//     // printf("only coalesce next\n");
//     p->next->size += p->size+sizeof(block_meta_t);
//     p->next->prev = p->prev;
//     if (p->prev) {
//       p->prev->next = p->next;
//     } else {
//       global_head = p->next;
//     }
//     total_memory_requested -= sizeof(block_meta_t);
//   }
// }

// void free(void *ptr) {
//     // implement free!
//     if (!ptr) return;
//     block_meta_t *p = (block_meta_t *)ptr - 1;
//     // assert(p->free == 0);
//     p->free = 1;
//     total_memory_requested -= p->size;
//     coalesceBlock(p);

//     // printf("free size: %zu\n", p->size);
//     // block_meta_t *temp = head;
//     // while(temp){
//       // printf("block->size: %zu, %d\n", temp->size, temp->free);
//     //   temp = temp->next;
//     // }
//     return;
// }

// int splitBlock(size_t size, block_meta_t *entry) {
//   if (entry->size >= 2*size && (entry->size-size) >= 1024) {
//     // printf("split block\n");
//     block_meta_t *new_entry = entry->ptr + size;
//     // printf("entry: %p\n", entry);
//     // printf("entry->ptr: %p\n", entry->ptr);
//     // printf("new_entry:     %p\n", new_entry);
//     // printf("entry->ptr end: %p\n", entry->ptr+entry->size);
//     new_entry->ptr = (new_entry + 1);
//     // printf("splitBlock segfault end\n");
//     new_entry->free = 1;
//     new_entry->size = entry->size - size - sizeof(block_meta_t);
//     new_entry->next = entry;
//     if (entry->prev) {
//       entry->prev->next = new_entry;
//     } else {
//       global_head = new_entry;
//     }
//     new_entry->prev = entry->prev;
//     entry->size = size;
//     entry->prev = new_entry;
//     // printf("entry->size: %zu\n", entry->size);
//     // printf("new_entry->size: %zu\n", new_entry->size);
//     // block_meta_t *temp = head;
//     // while(temp){
//     //   printf("block->size: %zu, %d\n", temp->size, temp->free);
//     //   temp = temp->next;
//     // }
//     return 1;
//   }
//   return 0;
// }

