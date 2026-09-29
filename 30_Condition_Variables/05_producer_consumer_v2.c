// Producer/Consumer, Single CV and If.
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
        if (count == 1) {
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
        if (count == 0) { // Very simple check, but it will cause issues when we have 1 Producer and 2 consumers- An Assertion failure, if C1 runs and sleeps, P produces, then wakes up C1. But before C1 can run, C2 gets scheduled, it consumes the buffer and makes counter 0. Now when C1 runs get(), it will cause assertion failure.
            Pthread_cond_wait(&cond, &mutex);
        }
        int tmp = get();
        Pthread_cond_signal(&cond);
        Pthread_mutex_unlock(&mutex);
        printf("%d\n", tmp);
    }
}


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