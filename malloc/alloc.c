// /**
//  * malloc
//  * CS 341 - Fall 2024
//  */
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>

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
//     // implement calloc!
//     return NULL;
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
//     // implement malloc!
//     return NULL;
// }

// /**
//  * Deallocate space in memory
//  *
//  * A block of memory previously allocated using a call to malloc(),
//  * calloc() or realloc() is deallocated, making it available again for
//  * further allocations.
//  *
//  * Notice that this function leaves the value of ptr unchanged, hence
//  * it still points to the same (now invalid) location, and not to the
//  * null pointer.
//  *
//  * @param ptr
//  *    Pointer to a memory block previously allocated with malloc(),
//  *    calloc() or realloc() to be deallocated.  If a null pointer is
//  *    passed as argument, no action occurs.
//  */
// void free(void *ptr) {
//     // implement free!
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
//     // implement realloc!
//     return NULL;
// }

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

/**
 * Metadata for each memory block.
 */
typedef struct block_meta {
    size_t size;
    struct block_meta *next;
    struct block_meta *prev;
    int free;
    int magic; // For debugging purposes
} block_meta_t;

#define META_SIZE sizeof(block_meta_t)

static block_meta_t *global_base = NULL;
static pthread_mutex_t global_malloc_lock = PTHREAD_MUTEX_INITIALIZER;

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
 * Finds a free block with enough space.
 */
block_meta_t *find_free_block(block_meta_t **last, size_t size) {
    block_meta_t *current = global_base;
    while (current) {
        if (current->free && current->size >= size) {
            return current;
        }
        *last = current;
        current = current->next;
    }
    return NULL;
}

/**
 * Extends the heap by requesting more memory from the OS.
 */
block_meta_t *request_space(block_meta_t* last, size_t size) {
    block_meta_t *block;
    block = sbrk(0);
    void *request = sbrk(size + META_SIZE);
    if (request == (void*) -1) {
        // sbrk failed.
        return NULL;
    }

    if (last) { // NULL on first request.
        last->next = block;
        block->prev = last;
    }

    block->size = size;
    block->next = NULL;
    block->free = 0;
    block->magic = 0x12345678;
    return block;
}

/**
 * Splits a block into two if it's larger than needed.
 */
void split_block(block_meta_t *block, size_t size) {
    block_meta_t *new_block = (void*)((char*)block + META_SIZE + size);
    new_block->size = block->size - size - META_SIZE;
    new_block->next = block->next;
    new_block->prev = block;
    new_block->free = 1;
    new_block->magic = 0x87654321;
    block->size = size;
    block->next = new_block;
    if (new_block->next) {
        new_block->next->prev = new_block;
    }
}

/**
 * Allocates memory block.
 */
void *malloc(size_t size) {
    block_meta_t *block;

    if (size <= 0) {
        return NULL;
    }

    size = align8(size);

    pthread_mutex_lock(&global_malloc_lock);

    if (!global_base) {
        // First call.
        block = request_space(NULL, size);
        if (!block) {
            pthread_mutex_unlock(&global_malloc_lock);
            return NULL;
        }
        global_base = block;
    } else {
        block_meta_t *last = global_base;
        block = find_free_block(&last, size);
        if (block) {
            // Found a free block
            if (block->size >= size + META_SIZE + 8) {
                // Split the block
                split_block(block, size);
            }
            block->free = 0;
            block->magic = 0x77777777;
        } else {
            // No suitable block found, request more memory
            block = request_space(last, size);
            if (!block) {
                pthread_mutex_unlock(&global_malloc_lock);
                return NULL;
            }
        }
    }

    pthread_mutex_unlock(&global_malloc_lock);
    return (block + 1); // Return pointer to data after metadata.
}

/**
 * Coalesces adjacent free blocks.
 */
void coalesce(block_meta_t *block) {
    // Coalesce with next block if it's free.
    if (block->next && block->next->free) {
        block->size += META_SIZE + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    // Coalesce with previous block if it's free.
    if (block->prev && block->prev->free) {
        block->prev->size += META_SIZE + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
        block = block->prev;
    }
}

/**
 * Deallocates space in memory.
 */
void free(void *ptr) {
    if (!ptr) {
        return;
    }

    pthread_mutex_lock(&global_malloc_lock);

    block_meta_t *block = (block_meta_t*)ptr - 1;
    block->free = 1;
    block->magic = 0x55555555;

    coalesce(block);

    pthread_mutex_unlock(&global_malloc_lock);
}

/**
 * Reallocates memory block.
 */
void *realloc(void *ptr, size_t size) {
    if (!ptr) {
        // Equivalent to malloc.
        return malloc(size);
    }

    if (size == 0) {
        // Equivalent to free.
        free(ptr);
        return NULL;
    }

    block_meta_t *block = (block_meta_t*)ptr - 1;

    if (block->size >= size) {
        // If the current block is large enough, reuse it.
        if (block->size >= size + META_SIZE + 8) {
            pthread_mutex_lock(&global_malloc_lock);
            split_block(block, align8(size));
            pthread_mutex_unlock(&global_malloc_lock);
        }
        return ptr;
    } else {
        // Try to merge with next free block if possible.
        pthread_mutex_lock(&global_malloc_lock);
        if (block->next && block->next->free && (block->size + META_SIZE + block->next->size) >= size) {
            coalesce(block);
            if (block->size >= size) {
                if (block->size >= size + META_SIZE + 8) {
                    split_block(block, align8(size));
                }
                pthread_mutex_unlock(&global_malloc_lock);
                return ptr;
            }
        }
        pthread_mutex_unlock(&global_malloc_lock);

        // Allocate new block.
        void *new_ptr = malloc(size);
        if (!new_ptr) {
            return NULL;
        }
        // Copy data to new block.
        memcpy(new_ptr, ptr, block->size);
        // Free old block.
        free(ptr);
        return new_ptr;
    }
}

/**
 * Allocates space for array in memory.
 */
void *calloc(size_t num, size_t size) {
    size_t total_size = num * size;
    void *ptr = malloc(total_size);
    if (!ptr) {
        return NULL;
    }
    // Initialize memory to zero.
    memset(ptr, 0, total_size);
    return ptr;
}
