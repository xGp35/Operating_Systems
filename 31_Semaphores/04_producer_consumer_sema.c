#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "common_threads.h"

#define MAX 10

int buffer[MAX];
int fill_ptr = 0;
int use_ptr = 0;
// We were using a num_full/count parameter to keep track of how many slots were full in the buffer


// pthread_mutex_t mutex;
// call pthread_mutex_init(&mutex, NULL); in main
// call pthread_mutex_destroy(&mutex, NULL); in main
// But you can instead use sem_init(&mutex, 0, 1);
sem_t mutex;

void put(int value) {
    sem_wait(&mutex);
    buffer[fill_ptr] = value;     // Line F1
    fill = (fill + 1) % MAX;      // Line F2
    sem_post(&mutex);
    // Here we used to increase the value of num_full. In the semaphore implementation, that is not requried.
}

void get() {
    sem_wait(&mutex);
    int tmp = buffer[use];    // Line G1
    use = (use + 1) % MAX;    // Line G2
    sem_post(&mutex);
    // Here we used to decrement num_full.
    return tmp;
}

sem_t empty;
sem_t full;

void* producer(void* arg) {
    int i;
    int loops = (int) arg;
    for(i=0; i<loops; i++) {
        sem_wait(&empty);       // Line P1
        put(i);                 // Line P2
        sem_post(&full);        // Line P3
    }
}

void* consumer(void* arg) {
    int tmp = 0;
    while (tmp != -1) {
        // Don't put a mutex lock here. This will cause a deadlock. More details below
        sem_wait(&full);        // Line C1
        tmp = get();            // Line C2
        sem_post(&empty);       // Line C3
        printf("%d\n", tmp);
    }
}

int main(int argc, char* argv[]) {
    //...
    //pthread_mutex_init(&mutex, NULL);

    sem_init(&empty, 0, MAX);   // MAX are empty
    sem_init(&full, 0, 0);      // 0 are full

    sem_init(&mutex, 0, 1);

    //pthread_mutex_destroy(&mutex, NULL);
    //...
}

/*
You've got the deadlock correctly identified — that's exactly it. Let me sharpen the mechanism, because there's a precise reason it happens, and it's the same precise reason your CV comparison is such a good instinct.

You asked "does it have to release the mutex, similar to how it releases the mutex when it goes into cond_wait?" — **the answer is yes, it *has to*, but `sem_wait` has no idea the mutex exists and does not do this for you.** That asymmetry is the whole bug.

Recall `pthread_cond_wait(&c, &m)` takes the mutex as an explicit argument. Its contract is specifically: atomically release `m` *and* sleep, as one indivisible step, then on waking, reacquire `m` before returning. That atomicity is built into the primitive precisely to prevent the producer/consumer from deadlocking this way — CVs were designed knowing they'd be used alongside a lock.

`sem_wait(&empty)` takes no mutex argument. It only knows about its own internal counter. It has absolutely no mechanism to release some other lock you happen to be holding — it can't, because it doesn't even know one exists. So trace it exactly as you did: producer acquires `mutex`, calls `sem_wait(&empty)`, counter goes to -1, producer blocks — **still holding `mutex`**, because nothing in `sem_wait`'s contract says to drop it. Now the consumer, who needs `mutex` to even attempt `get()` and free up a slot, can never acquire it. Permanent deadlock — the producer is asleep waiting for a signal that only the consumer can send, and the consumer is asleep (blocked on the mutex) waiting for the producer to let go of something it will never let go of while asleep.

So the fix isn't "the producer should release the mutex before blocking" as a behavior you'd code in — it's "don't put the mutex in a position where the thread could block on something else while still holding it." Concretely: call `sem_wait(&empty)` *before* acquiring the mutex, so by the time you're holding the lock, you've already secured a slot and `put()` can't block.

The general principle this generalizes to, worth keeping as a standing rule: **never block (sleep, wait, or do I/O) while holding a lock**, unless the primitive you're blocking on explicitly knows about that lock and releases it for you (as `cond_wait` does). That's a real, common bug class outside of textbooks too.
*/