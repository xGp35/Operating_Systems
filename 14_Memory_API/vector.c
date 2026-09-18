#include <stdio.h>
#include <stdlib.h>

/*
 * Our own little vector data structure.
 *
 * data     -> points to the dynamically allocated array
 * size     -> how many elements are currently stored
 * capacity -> how many elements can fit in the allocated array
 *
 * Example:
 *
 *     data
 *       |
 *       v
 *     [10][20][30][  ][  ][  ][  ]
 *      <------ size ---->
 *      <--------- capacity -------->
 *
 * size = 3
 * capacity = 7
 */
typedef struct {
    int *data;
    size_t size;
    size_t capacity;
} Vector;


/*
 * Initialize an empty vector.
 *
 * Initially we don't allocate any memory.
 * We will allocate memory later when the first element
 * is inserted.
 */
void vector_init(Vector *v) {
    v->data = NULL;      // No array allocated yet
    v->size = 0;         // No elements
    v->capacity = 0;     // No space allocated
}


/*
 * Add one element to the end of the vector.
 */
void vector_push_back(Vector *v, int value) {

    /*
     * Is the vector full?
     *
     * Example:
     *
     * size = 4
     * capacity = 4
     *
     * There is no room for another element,
     * so we need to grow the array.
     */
    if (v->size == v->capacity) {

        /*
         * Increase the capacity.
         *
         * If this is the first allocation:
         *
         *     0 -> 1
         *
         * Otherwise, double the capacity:
         *
         *     1 -> 2 -> 4 -> 8 -> 16 -> ...
         *
         * Doubling is important because it avoids calling
         * realloc() every time we insert an element.
         */
        size_t new_capacity = (v->capacity == 0) ? 1 : v->capacity * 2;

        /*
         * Resize the existing memory allocation.
         *
         * realloc() will try to give us enough memory for
         * new_capacity integers.
         *
         * It may be able to extend the existing block,
         * OR it may allocate a new block, copy the old
         * contents there, and release the old block.
         *
         * Either way, new_data points to the resulting array.
         */
        int *new_data = realloc( v->data, new_capacity * sizeof(int) );


        /*
         * realloc() returns NULL if the allocation fails.
         *
         * IMPORTANT:
         * We haven't overwritten v->data yet.
         *
         * That's good practice because if realloc fails,
         * our original allocation is still accessible through
         * v->data.
         */
        if (new_data == NULL) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }


        /*
         * realloc() succeeded.
         *
         * Update our vector so that it points to the
         * newly resized array.
         */
        v->data = new_data;

        /*
         * Remember how much space we now have.
         */
        v->capacity = new_capacity;
    }


    /*
     * There is now guaranteed to be space for the new element.
     *
     * v->size is the index where the new element should go.
     *
     * Example:
     *
     * size = 3
     *
     *     [10][20][30][  ][  ]
     *                 ^
     *                 |
     *              v->data[3]
     */
    v->data[v->size] = value;


    /*
     * We just added one element,
     * so increase the number of elements stored.
     */
    v->size++;
}


/*
 * Free all memory owned by the vector.
 */
void vector_free(Vector *v) {

    /*
     * Release the dynamically allocated array.
     */
    free(v->data);

    /*
     * Reset everything so the Vector is back to
     * an empty/safe state.
     */
    v->data = NULL;
    v->size = 0;
    v->capacity = 0;
}


int main(void) {

    /*
     * Create a Vector variable.
     *
     * At this point, it exists on the stack.
     * The actual array that stores the integers
     * will live on the heap.
     */
    Vector v;


    /*
     * Initialize the vector.
     *
     * After this:
     *
     * v.data     = NULL
     * v.size     = 0
     * v.capacity = 0
     */
    vector_init(&v);


    /*
     * Add 1,000,000 integers to the vector.
     *
     * vector_push_back() will automatically grow
     * the underlying array whenever it becomes full.
     *
     * The capacity will grow approximately like:
     *
     * 1
     * 2
     * 4
     * 8
     * 16
     * ...
     * 524288
     * 1048576
     *
     * So we DON'T call realloc() one million times.
     */
    for (int i = 0; i < 1000000; i++) {
        vector_push_back(&v, i);
    }


    /*
     * size tells us how many elements we actually have.
     *
     * Should be:
     *
     * size = 1000000
     */
    printf("size = %zu\n", v.size);


    /*
     * capacity tells us how much space we allocated.
     *
     * This will be >= size.
     *
     * Because we double the capacity, it will likely be:
     *
     * capacity = 1048576
     */
    printf("capacity = %zu\n", v.capacity);


    /*
     * We're finished with the vector.
     *
     * Release the heap memory.
     *
     * Without this, Valgrind would report a memory leak.
     */
    vector_free(&v);


    return 0;
}