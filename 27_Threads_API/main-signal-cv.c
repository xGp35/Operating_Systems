#include <stdio.h>

#include "common_threads.h"

// 
// simple synchronizer: allows one thread to wait for another
// structure "synchronizer_t" has all the needed data
// methods are:
//   init (called by one thread)
//   wait (to wait for a thread)
//   done (to indicate thread is done)
// 
typedef struct __synchronizer_t {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int done;
} synchronizer_t;
//If you wanted a second, unrelated producer/consumer pair to synchronize somewhere else in the same program, you'd need a second global variable, and a second lock, and you'd have to remember by convention which lock goes with which variable. Nothing in the code enforces that pairing — it lives only in the programmer's head and in comments.

//With synchronizer_t, the lock, the condition variable, and the data they protect are bundled into one unit that gets passed around by pointer:


synchronizer_t s;

// Signal initializer
void signal_init(synchronizer_t* s) {
    Pthread_mutex_init(&s->lock, NULL);
    Pthread_cond_init(&s->cond, NULL);
    s->done = 0;
}

// Signal destroyer
void signal_destroy(synchronizer_t *s) {
    Pthread_mutex_destroy(&s->lock);
    Pthread_cond_destroy(&s->cond);
}


// Signaller block
void signal_done(synchronizer_t *s) {
    Pthread_mutex_lock(&s->lock);
    s->done = 1;
    Pthread_cond_signal(&s->cond);
    Pthread_mutex_unlock(&s->lock);
}

void signal_wait(synchronizer_t *s) {
    Pthread_mutex_lock(&s->lock);
    while (s->done == 0) // The braces are optional when the body of a while loop contains exactly one statement.
	Pthread_cond_wait(&s->cond, &s->lock);
    Pthread_mutex_unlock(&s->lock);
}

void* worker(void* arg) {
    printf("this should print first\n");
    signal_done(&s);
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t p;
    signal_init(&s);
    Pthread_create(&p, NULL, worker, NULL);
    signal_wait(&s);
    Pthread_join(p, NULL);      // missing
    signal_destroy(&s);         // was also missing
    printf("this should print last\n");

    return 0;
}