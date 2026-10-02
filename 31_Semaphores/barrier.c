#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common_threads.h"

// If done correctly, each child should print their "before" message
// before either prints their "after" message. Test by adding sleep(1)
// calls in various locations.

// You likely need two semaphores to do this correctly, and some
// other integers to track things.

typedef struct __barrier_t {
    // Two turnstiles, one for arrival, one for departure
    sem_t mutex;
    sem_t turnstile1;  // Turnstile 1 gates arrival: nobody passes until the Nth arriver opens the gate.
    sem_t turnstile2;  // Turnstile 2 gates departure: the mirror image, opened only once everyone's count has drained back to 0.
    int count; // This is num_arrived
    int N; // This is num_threads
} barrier_t;

// the single barrier we are using for this program
barrier_t b;

void barrier_init(barrier_t *b, int num_threads) {
    sem_init(&b->mutex, 0, 1);
    sem_init(&b->turnstile1, 0, 0);
    sem_init(&b->turnstile2, 0, 0);
    b->count = 0;
    b->N = num_threads;
}

void barrier_wait(barrier_t *b) { // barrier_wait
    // ----------Phase 1: Arrival------------
    sem_wait(&b->mutex); // This is the counter mutex. Used for mutual exclusive increase in counter, so that 2 threads don't scribble ove the counter (b->count)
    b->count++;
    if (b->count == b->N) {
        for (int i = 0; i < b->N; i++) {// In C I can't directly set the value of the semaphore to N, so I loop here to increase the value till N
            sem_post(&b->turnstile1);
        }
    }
    sem_post(&b->mutex);  // release the count mutex
    sem_wait(&b->turnstile1); // This is the gate where each Thread waits in the First phase till all of its peers have arrived. Once all have arrived, the last arriving peer increases the tokens at this tunstile by N, which enables all of the waiting threads to pass through this turnstile.

    // Critical Point- This is the point where I need all of the threads to arrive together. Now they arrive here together. Basically they arrive at this point in code after all are done.

    // ----------Phase 2: Departure------------
    // The critcal point is maybe being accesses in a loop. So to leave the state of the turnstile and count in the exact same manner as which the first loop happened, we ahve to restore the counter back to 0 (now its N) and make sure turnstile1 is 0. We add a second tuenstile now.
    
    sem_wait(&b->mutex);
    b->count--;
    if (b->count == 0) {
        for (int i = 0; i < b->N; i++)
            sem_post(&b->turnstile2); // Again we wanted to set the value to N, but because this is C, we have to run it N times to increase its value to N. This will go back to 0, once Line X is executed.
    }
    sem_post(&b->mutex); // release the count mutex

    sem_wait(&b->turnstile2); // Line X- Final Exit
}

//
// XXX: don't change below here (just run it!)
//
typedef struct __tinfo_t {
    int thread_id;
} tinfo_t;

void *child(void *arg) {
    tinfo_t *t = (tinfo_t *) arg;
    printf("child %d: before\n", t->thread_id);
    barrier_wait(&b);
    printf("child %d: after\n", t->thread_id);
    return NULL;
}


// run with a single argument indicating the number of 
// threads you wish to create (1 or more)
int main(int argc, char *argv[]) {
    assert(argc == 2);
    int num_threads = atoi(argv[1]);
    assert(num_threads > 0);

    pthread_t p[num_threads];
    tinfo_t t[num_threads];

    printf("parent: begin\n");
    barrier_init(&b, num_threads);
    
    int i;
    for (i = 0; i < num_threads; i++) {
        t[i].thread_id = i;
        Pthread_create(&p[i], NULL, child, &t[i]);
    }

    for (i = 0; i < num_threads; i++) 
	    Pthread_join(p[i], NULL);

    printf("parent: end\n");
    return 0;
}

/*
class Barrier:
    def __init__(self, n):
        self.n = n
        self.count = 0
        self.mutex = Semaphore(1)
        self.turnstile = Semaphore(0)
        self.turnstile2 = Semaphore(0)

    def phase1(self):
        self.mutex.wait()
        self.count += 1
        if self.count == self.n:
            self.turnstile.signal(self.n)
        self.mutex.signal()
        self.turnstile.wait()
    
    def phase2(self):
        self.mutex.wait()
        self.count -= 1
        if self.count == 0:
            self.turnstile2.signal(self.n)
        self.mutex.signal()
        self.turnstile2.wait()
    
    def wait(self):
        self.phase1()
        self.phase2()
*/