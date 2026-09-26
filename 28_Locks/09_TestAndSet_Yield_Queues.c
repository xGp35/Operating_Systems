#include <stdio.h>

#include "common_threads.h"

int TestAndSet(int *old_ptr, int new) {
    int old = *old_ptr;   // fetch old value at old_ptr
    *old_ptr = new;       // store 'new' into old_ptr
    return old;           // return the old value
}

typedef struct __lock_t { 
    int flag;
    int guard;
    queue_t *q;
} queue_t;

typedef struct __lock_t { 
    int flag;
    int guard;
    queue_t *q;
} lock_t;

void lock_init(lock_t *mutex) {
    mutex->flag = 0;
    mutex->guard = 0;
    queue_init(mutex->q);
}

void lock(lock_t *mutex) {
    while (TestAndSet(&mutex->guard, 1) == 1)
        ; // acquire guard lock by spinning
    if (mutex->flag == 0) {
        mutex->flag = 1; //lock is acquired
        mutex->guard = 0; // guard would have been set to 1 , if it was 0 initially, in the TestAndSet function call, now we reset it to 0
    } else {
        queue_add(mutex->q, getid()); // Add Thread2 to the queue, as Thread 1 has the lock.
        mutex->guard = 0;
        park(); // put calling thread to sleep, so Thread 2 goes to sleep
    }
}

void unlock(lock_t *mutex) {
    while (TestAndSet(&mutex->guard, 1) == 1) 
        ; // acquire guard lock by spinning
    if (queue_empty(mutex->q))
        mutex->flag = 0; // let go of lock; no one wants it
    else
        unpark(queue_remove(mutex->q)); //wake the first thread in the queue.
    
    mutex->guard = 0;
}

// Thread 2 had called lock and had been put in front of  queue and then put to sleep.
// Now when Thread 1 calls unlock, unpark is called on the first item of the queue. ie, Thread 2
// Now Thread 2, continues executing, the flag is still set to 1, it was never set to 0.
// This is done to avoid thundering herd problem. Once the queue is empty, then the flag will be
// set to 0.