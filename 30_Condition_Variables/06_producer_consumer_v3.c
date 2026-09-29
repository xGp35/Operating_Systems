// Producer/Consumer, Single CV and While.
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "common_threads.h"

int buffer;
int count = 0;

int loops;
cond_t cond;
mutex_t mutex;

void* producer(void* arg) {
    int i;
    loops = (int) arg;
    for (i = 0; i < loops; i++) {
        Pthread_mutex_lock(&mutex);
        while (count == 1) {
            Pthread_cond_wait(&cond, &mutex);
        }
        put(i);
        Pthread_cond_signal(&cond);
        Pthread_mutex_unlock(&mutex);
    }
}

void* consumer(void* arg) {
    int i;
    loops = (int) arg;
    for (i = 0; i < loops; i++) {
        Pthread_mutex_lock(&mutex);
        while (count == 0) { // We repalce while with a for to double check. This is because every system uses Mesa semantics - Signalling is a HINT that the state of the world has changed. It doesn't gurantee that, when a woken thread runs, the state of the world will be the same as when it was woken up. While loop will avoid the assertion error in producer_consumer_v2. But it has its own problems.
            Pthread_cond_wait(&cond, &mutex);
        }
        int tmp = get();
        Pthread_cond_signal(&cond);
        Pthread_mutex_unlock(&mutex);
        printf("%d\n", tmp);
    }
}
// Hoare sematics gave stronger gurantee, but no one uses it. This basically said, woken thread will be made to run immediately. Hard to implement.

// Problem of v3. We are using a single condition variable. This can cause issues when C1 runs, goes to wait, c2 runs goes to wait. Then producer produces, wakes up C1. C1 consumes. Then wakes up C2 (which was next in queue. only woken not scheduled). Then C1 tries consuming again. Sees empty buffer (count = 0), goes to sleep. C2 gets scheduled.It will see count = 0 as C1 consumed the produced value. It will sleep. All 3 are sleeping. SAD !!
// We need more directed conditional variables. 
// Consumers should not wake up other consumers. It should only wake up producer. Vice versa.


void put (int value) {
    assert(count == 0); // Put assumes the buffer is empty, as the buffer capacity is 1.
    count = 1;
    buffer = value;
}

int get() {
    assert(count == 1); 
    count = 0;
    return buffer;
}