#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>

#include "common.h"
#include "common_threads.h"
#include "pc-header.h"

pthread_cond_t empty  = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill   = PTHREAD_COND_INITIALIZER;
pthread_mutex_t m     = PTHREAD_MUTEX_INITIALIZER;

#include "main-header.h"

void do_fill(int value) {
    // ensure empty before usage
    ensure(buffer[fill_ptr] == EMPTY, "error: tried to fill a non-empty buffer"); 
    // This is not checking whether the entire buffer is empty. It's checking whether the specific slot at fill_ptr is empty. "Ensure" prints the error message if the condition evaluated to False. If there is no free space available, then it will print the error message. And we need it to be empty because this is the producer. Got it
    buffer[fill_ptr] = value;
    fill_ptr = (fill_ptr + 1) % max;
    num_full++;
}

int do_get() {
    int tmp = buffer[use_ptr];
    ensure(tmp != EMPTY, "error: tried to get an empty buffer");
    buffer[use_ptr] = EMPTY; 
    use_ptr = (use_ptr + 1) % max;
    num_full--;
    return tmp;
}

void *producer(void *arg) {
    int id = (int)(long long) arg;
    // make sure each producer produces unique values
    int base = id * loops; 
    int i;
    for (i = 0; i < loops; i++) {   p0;
        Mutex_lock(&m);             p1;
        if (num_full == max) {      p2;
            Cond_wait(&empty, &m);  p3;
        }
        do_fill(base + i);          p4;
        Cond_signal(&fill);         p5;
        Mutex_unlock(&m);           p6;
    }
    return NULL;
}
                                                                               
void *consumer(void *arg) {
    int id = (int)(long long) arg;
    int tmp = 0;
    int consumed_count = 0;
    while (tmp != END_OF_STREAM) { c0;
        Mutex_lock(&m);            c1;
        if (num_full == 0) {       c2;
            Cond_wait(&fill, &m);  c3;
            }
        tmp = do_get();            c4;
        Cond_signal(&empty);       c5;
        Mutex_unlock(&m);          c6;
	consumed_count++;
    }

    // return consumer_count-1 because END_OF_STREAM does not count
    return (void *) (long long) (consumed_count - 1);
}

// must set these appropriately to use "main-common.c"
pthread_cond_t *fill_cv = &fill;
pthread_cond_t *empty_cv = &empty;

// all codes use this common base to start producers/consumers
// and all the other related stuff
#include "main-common.c"


