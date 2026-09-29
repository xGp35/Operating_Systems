// Pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m);
// Pthread_cond_signal(pthread_cond_t *c);

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "common_threads.h"

int done = 0;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; // TODO: change this to pthread_mutex_init and destroy
pthread_cond_t c = PTHREAD_COND_INITIALIZER;

void thr_exit() {
    Phthread_mutex_lock(&m);
    done = 1;
    Pthread_cond_signal(&c); // reaches in to condition variables own wait queue and wakes one thread on it
    Pthread_mutex_unlock(&m);
}

void* child(void* arg) {
    printf("child\n");
    thr_exit(); // Immediately after being called this calls the exit method, which is supposed to exit out of this?
    return NULL;
}

void thr_join() {
    Pthread_mutex_lock(&m);
    while (done == 0) {
        Pthread_cond_wait(&c, &m);
    }
    Pthread_mutex_unlock(&m);
}

int main(int argc, char* argv[]) {
    printf("parent: begin\n");
    pthread_t p;
    Pthread_create(&p, NULL, child, NULL);
    thr_join();
    printf("parent: end\n");
    return 0;
}