#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "common_threads.h"

#define NUMCPUS 4 

typedef struct __counter_t {
    int global;                         // global count
    pthread_mutex_t glock;              // global lock
    int local[NUMCPUS];                 //per-CPU count
    pthread_mutex_t llock[NUMCPUS];     //per-CPU locks
    int threshold;                       // called S in the book. Its the update frequency
} counter_t;

// init: record (a)threshold, (b)init locks, (c)init values of all local counts and global count
void init(counter_t *c, int threshold) {
    c->threshold = threshold;
    c->global = 0;
    Pthread_mutex_init(&c->glock, NULL); // Normally I used to write "&mutex" here.
    int i;
    for (i=0; i< NUMCPUS; i++) {
        c->local[i] = 0;
        Pthread_mutex_init(&c->llock[i], NULL);
    }
}

// update: usually , just grab local lock and update local amount;once it has risen to "threshold",
// grab global lock and transfer local values to it.
void update(counter_t *c, int threadID, int amt) {
    int cpu = threadID % NUMCPUS;  // Is this hashing? like which CPU will a particular thread ID go to? - YEAH !!
    Pthread_mutex_lock(&c->llock[cpu]);
    c->local[cpu] += amt;
    if (c->local[cpu] >= c->threshold) {
        // transfer to global (assumes amt > 0)
        Pthread_mutex_lock(&c->glock);
        c->global += c->local[cpu];
        Pthread_mutex_unlock(&c->glock);
        c->local[cpu] = 0;
    }
    Pthread_mutex_unlock(&c->llock[cpu]);
}

// get: just return global amount (approximate)
int get(counter_t *c) {
    Pthread_mutex_lock(&c->glock);
    int val = c->global;
    Pthread_mutex_unlock(&c->glock);
    return val; // only approximate
}