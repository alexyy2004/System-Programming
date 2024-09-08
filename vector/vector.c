/**
 * vector
 * CS 341 - Fall 2024
 */
#include "vector.h"
#include <assert.h>

struct vector {
    /* The function callback for the user to define the way they want to copy
     * elements */
    copy_constructor_type copy_constructor;

    /* The function callback for the user to define the way they want to destroy
     * elements */
    destructor_type destructor;

    /* The function callback for the user to define the way they a default
     * element to be constructed */
    default_constructor_type default_constructor;

    /* Void pointer to the beginning of an array of void pointers to arbitrary
     * data. */
    void **array;

    /**
     * The number of elements in the vector.
     * This is the number of actual objects held in the vector,
     * which is not necessarily equal to its capacity.
     */
    size_t size;

    /**
     * The size of the storage space currently allocated for the vector,
     * expressed in terms of elements.
     */
    size_t capacity;
};

/**
 * IMPLEMENTATION DETAILS
 *
 * The following is documented only in the .c file of vector,
 * since it is implementation specfic and does not concern the user:
 *
 * This vector is defined by the struct above.
 * The struct is complete as is and does not need any modifications.
 *
 * The only conditions of automatic reallocation is that
 * they should happen logarithmically compared to the growth of the size of the
 * vector inorder to achieve amortized constant time complexity for appending to
 * the vector.
 *
 * For our implementation automatic reallocation happens when -and only when-
 * adding to the vector makes its new  size surpass its current vector capacity
 * OR when the user calls on vector_reserve().
 * When this happens the new capacity will be whatever power of the
 * 'GROWTH_FACTOR' greater than or equal to the target capacity.
 * In the case when the new size exceeds the current capacity the target
 * capacity is the new size.
 * In the case when the user calls vector_reserve(n) the target capacity is 'n'
 * itself.
 * We have provided get_new_capacity() to help make this less ambigious.
 */

static size_t get_new_capacity(size_t target) {
    /**
     * This function works according to 'automatic reallocation'.
     * Start at 1 and keep multiplying by the GROWTH_FACTOR untl
     * you have exceeded or met your target capacity.
     */
    size_t new_capacity = 1;
    while (new_capacity < target) {
        new_capacity *= GROWTH_FACTOR;
    }
    return new_capacity;
}

vector *vector_create(copy_constructor_type copy_constructor,
                      destructor_type destructor,
                      default_constructor_type default_constructor) {
    // your code here
    // Casting to void to remove complier error. Remove this line when you are
    // ready.
    (void)INITIAL_CAPACITY;
    (void)get_new_capacity;
    return NULL;
}

void vector_destroy(vector *this) {
    assert(this);
    // your code here
}

void **vector_begin(vector *this) {
    return this->array + 0;
}

void **vector_end(vector *this) {
    return this->array + this->size;
}

size_t vector_size(vector *this) {
    assert(this);
    // your code here
    return 0;
}

void vector_resize(vector *this, size_t n) {
    assert(this);
    // your code here
}

size_t vector_capacity(vector *this) {
    assert(this);
    // your code here
    return 0;
}

bool vector_empty(vector *this) {
    assert(this);
    // your code here
    return true;
}

void vector_reserve(vector *this, size_t n) {
    assert(this);
    // your code here
}

void **vector_at(vector *this, size_t position) {
    assert(this);
    // your code here
    return NULL;
}

void vector_set(vector *this, size_t position, void *element) {
    assert(this);
    // your code here
}

void *vector_get(vector *this, size_t position) {
    assert(this);
    // your code here
    return NULL;
}

void **vector_front(vector *this) {
    assert(this);
    // your code here
    return NULL;
}

void **vector_back(vector *this) {
    // your code here
    return NULL;
}

void vector_push_back(vector *this, void *element) {
    assert(this);
    // your code here
}

void vector_pop_back(vector *this) {
    assert(this);
    // your code here
}

void vector_insert(vector *this, size_t position, void *element) {
    assert(this);
    // your code here
}

void vector_erase(vector *this, size_t position) {
    assert(this);
    assert(position < vector_size(this));
    // your code here
}

void vector_clear(vector *this) {
    // your code here
}
