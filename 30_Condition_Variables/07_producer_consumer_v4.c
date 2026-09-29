// From producer_consumer_v3, we learnt that We need more directed conditional variables. 
// Consumers should not wake up other consumers. It should only wake up producer. Vice versa.
// Producer/Consumer, Two CV and While.
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "common_threads.h"

int buffer;
int count = 0;

int loops;
cond_t empty, full;
mutex_t mutex;

void* producer(void* arg) {
    int i;
    loops = (int) arg;
    for (i = 0; i < loops; i++) {
        Pthread_mutex_lock(&mutex);
        while (count == 1) {
            Pthread_cond_wait(&empty, &mutex); // This is waiting for a empty signal from consumer, who is responsible for emptying the buffer, so that it can fill it.
        }
        put(i);
        Pthread_cond_signal(&full); // Once it makes the buffer full, it can say that "Hey Consumers, who are waiting for buffer to be full. It's now full. Go Play!!"
        Pthread_mutex_unlock(&mutex);
    }
}

void* consumer(void* arg) {
    int i;
    loops = (int) arg;
    for (i = 0; i < loops; i++) {
        Pthread_mutex_lock(&mutex);
        while (count == 0) {
            Pthread_cond_wait(&full, &mutex); // Consumer is waiting on the full signal. If full is signalled by the producer, it will wake up and get to work. So you can think of it like this. count == 0, so nothing in buffer, ie buffer is empty. I need to wait for a signal that says "Buffer is Full"
        }
        int tmp = get();
        Pthread_cond_signal(&empty);
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