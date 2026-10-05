#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "common_threads.h"

#include "main-header.h"
#include "vector-header.h"

void vector_add(vector_t *v_dst, vector_t *v_src) {
    Pthread_mutex_lock(&v_dst->lock);
    Pthread_mutex_lock(&v_src->lock);
    int i;
    for (i = 0; i < VECTOR_SIZE; i++) {
	v_dst->values[i] = v_dst->values[i] + v_src->values[i];
    }
    Pthread_mutex_unlock(&v_dst->lock);
    Pthread_mutex_unlock(&v_src->lock);
}

void fini() {}

#include "main-common.c"

/*
There are 4 conditions of deadlock.
1. Mutual exclusivity - Threads claim exclusive control of the resources that they requrie.
2. Hold-and-wait - Threads hold on to the resources that they have acquired, while waiting for something else.
3. No Preemption by CPU. - Resources cannot be removed from the thread by the CPU.
4. Circular dependency - There exists a circular chain of dependency, such that each thread holds on to the reousre requried by next thread in the chain.
*/