#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "common_threads.h"

int buffer;
int count = 0;

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
// Very wrong, can cause a lot of problems
void* producer(void* arg) {
    int i;
    int loops = (int) arg;
    for (i = 0; i < loops; i++) {
        put(i);
    }
}

void* consumer(void* arg) {
    while (1) {
        int tmp = get();
        printf("%d\n", tmp);
    }
}