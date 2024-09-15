/**
 * vector
 * CS 341 - Fall 2024
 */
#include "vector.h"
#include <assert.h>
#include <stdio.h>

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
    vector *vec = (vector *)calloc(1, sizeof(vector));
    if (copy_constructor == NULL) {
        vec->copy_constructor = shallow_copy_constructor;
    } else {
        vec->copy_constructor = copy_constructor;
    }
    if (destructor == NULL) {
        vec->destructor = shallow_destructor;
    } else {
        vec->destructor = destructor;
    }
    if (default_constructor == NULL) {
        vec->default_constructor = shallow_default_constructor;
    } else {
        vec->default_constructor = default_constructor;
    }
    size_t size = 0;
    size_t capacity = INITIAL_CAPACITY;
    void **array = (void **)calloc(0, sizeof(void *)); // initialize the elements in array to be 0
    vec->size = size;
    vec->capacity = capacity;
    vec->array = array;
    return vec;
}

void vector_destroy(vector *this) {
    assert(this);
    for (size_t i = 0; i < this->size; i++) {
        this->destructor(this->array[i]);
    }
    free(this->array);
    free(this);
    // your code here
}

void **vector_begin(vector *this) {
    if (this->size == 0) {
        return NULL; // If the container is empty, the returned iterator value shall not be dereferenced.
    } else {
        return this->array;
    }
    // return this->array + 0;
}

void **vector_end(vector *this) {
    return this->array + this->size;
}

size_t vector_size(vector *this) {
    assert(this);
    // your code here
    return this->size;
}

void vector_resize(vector *this, size_t n) {
    assert(this);
    // your code here
    // if (n < this->size) {
    //     for (size_t i = n; i < this->size; i++) {
    //         this->destructor(this->array[i]);
    //     }
    //     this->size = n;
    // } else if (n > this->size) {
    //     if (n > this->capacity) {
    //         vector_reserve(this, n);
    //     }
    //     for (size_t i = this->size; i < n; i++) {
    //         this->array[i] = this->default_constructor();
    //     }
    //     this->size = n;
    // }
    
    if (n < this->size) {
        for (size_t i = n; i < this->size; i++) {
            this->destructor(this->array[i]);
        }
        this->size = n;
        return;
    }

    if (n > this->size) {
        if (n <= this->capacity) {
            for (size_t i = this->size; i < this->capacity; i++) {
                this->array[i] = this->default_constructor();
            }
        } else {
            vector_reserve(this, n);
        }
        this->size = n;
    }

}

size_t vector_capacity(vector *this) {
    assert(this);
    // your code here
    return this->capacity;
}

bool vector_empty(vector *this) {
    assert(this);
    // your code here
    return this->size == 0;
}

void vector_reserve(vector *this, size_t n) {
    assert(this);
    // your code here
    if (n > this->capacity) {
        size_t new_capacity = get_new_capacity(n);
        // this->array = realloc(this->array, new_capacity * sizeof(void *));
        void **new_array = (void **)calloc(new_capacity, sizeof(void *));
        for (size_t i = 0; i < this->size; i++) {
            new_array[i] = this->array[i];
        }
        free(this->array);
        this->array = new_array;
        
        this->capacity = new_capacity;
    } else {

    }
}

void **vector_at(vector *this, size_t position) {
    assert(this);
    // your code here
    assert(this->size > position);
    return this->array + position;
}

void vector_set(vector *this, size_t position, void *element) {
    assert(this);
    // your code here
    assert(this->size > position);
    void *cur_ele = this->array[position];
    void **cur_arr = this->array + position;
    *cur_arr = this->copy_constructor(element);
    this->destructor(cur_ele);
}

void *vector_get(vector *this, size_t position) {
    assert(this);
    // your code here
    assert(this->size > position);
    return (this->array)[position];
}

void **vector_front(vector *this) {
    assert(this);
    // your code here
    return this->array;
}

void **vector_back(vector *this) {
    // your code here
    return this->array + (this->size - 1);
}

void vector_push_back(vector *this, void *element) {
    printf("%s\n", (char*)element);
    assert(this);
    // your code here
    if (this->size + 1 > this->capacity) {
        vector_reserve(this, this->size + 1);
        this->capacity = get_new_capacity(this->size);
    }
    this->array[this->size] = (this->copy_constructor)(element);
    this->size += 1;
}

void vector_pop_back(vector *this) {
    assert(this);
    // your code here
    this->destructor(this->array[this->size - 1]);
    this->size -= 1;
}

void vector_insert(vector *this, size_t position, void *element) {
    assert(this);
    // your code here
    if (this->size + 1 > this->capacity) {
        vector_reserve(this, this->size + 1);
        this->capacity = get_new_capacity(this->size);
    }
    for (size_t i = this->size; i > position; i--) {
        this->array[i] = this->array[i-1];
    }
    this->array[position] = this->copy_constructor(element);   
    this->size += 1;
}

void vector_erase(vector *this, size_t position) {
    assert(this);
    assert(position < vector_size(this));
    // your code here
    this->destructor(this->array[position]);
    if (position == this->size - 1) {
        this->size -= 1;
    } else {
        for (size_t i = position; i < this->size - 1; i++) {
            this->array[i] = this->array[i + 1];
        }
        this->size -= 1;
    }
}

void vector_clear(vector *this) {
    // your code here
    for (size_t i = 0; i < this->size; i++) {
        this->destructor(this->array[i]);
    }
    this->size = 0;
}
