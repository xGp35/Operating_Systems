#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "common_threads.h"

#include "main-header.h"
#include "vector-header.h"

void vector_add(vector_t *v_dst, vector_t *v_src) {
    if (v_dst < v_src) { // memory addresses of v_dst and v_src (v_dst < v_src)
        Pthread_mutex_lock(&v_dst->lock);
        Pthread_mutex_lock(&v_src->lock);
    } else if (v_dst > v_src) {
        Pthread_mutex_lock(&v_src->lock);
        Pthread_mutex_lock(&v_dst->lock);
    } else {
        // special case: src and dst are the same
        Pthread_mutex_lock(&v_src->lock); // We don't try to acquire the lock 2 times, because that is also a deadlock condition. If v_dst and v_src point to the same struct, then &v_dst->lock and &v_src->lock are the same memory address — the same mutex. The thread deadlocks with itself.
    }
    int i;
    for (i = 0; i < VECTOR_SIZE; i++) {
        v_dst->values[i] = v_dst->values[i] + v_src->values[i];
    }
    Pthread_mutex_unlock(&v_src->lock);
    if (v_dst != v_src) 
	    Pthread_mutex_unlock(&v_dst->lock);
}

void fini() {}


#include "main-common.c"

/*
 The code compares the memory addresses of v_dst and v_src (v_dst < v_src). The lower-address vector always gets locked first, regardless of which argument position it was passed in. So even with -d where thread 0 calls vector_add(v[0], v[1]) and thread 1 calls vector_add(v[1], v[0]), both threads lock v[0] first (because it's at a lower memory address). Same order → no cycle → no deadlock. This breaks the circular wait condition.
*/

/*
A mutex in C is just a variable — it doesn't "belong" to an object the way a lock might in Java or Python's threading.Lock() attached to a class. But that doesn't matter. What matters is: is it the same mutex? If v_dst and v_src point to the same struct, then &v_dst->lock and &v_src->lock are the same memory address — the same mutex. Calling pthread_mutex_lock on it twice from the same thread = self-deadlock,
*/