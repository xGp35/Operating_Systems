// We want more buffer size.

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "common_threads.h"

#define MAX 10

int buffer[MAX];
int fill_ptr = 0;
int use_ptr = 0;
int count = 0;

// Put and get routines have been modified to provide better handling of buffer array with a max capacity.
void put (int value) {
    buffer[fill_ptr] = value;
    fill_ptr = (fill_ptr + 1) % MAX;
    count++;
}

int get() {
    int tmp = buffer[use_ptr];
    use_ptr = (use_ptr + 1) % MAX;
    count--;
    return tmp;
}

cond_t empty, full;
mutex_t mutex;

void* producer(void* arg) {
    int i;
    int loops = (int) arg;
    for (i = 0; i < loops; i++) {
        Pthread_mutex_lock(&mutex);
        while (count == MAX) {
            Pthread_cond_wait(&empty, &mutex); // This is waiting for a empty signal from consumer, who is responsible for emptying the buffer, so that it can fill it.
        }
        put(i);
        Pthread_cond_signal(&full); // Once it makes the buffer full, it can say that "Hey Consumers, who are waiting for buffer to be full. It's now full. Go Play!!"
        Pthread_mutex_unlock(&mutex);
    }
}

void* consumer(void* arg) {
    int i;
    int loops = (int) arg;
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


