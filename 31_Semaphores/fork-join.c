#include <stdio.h>
#include <unistd.h>
#include "common_threads.h"

sem_t s;

void *child(void* arg) {
    printf("child\n");
    sleep(1); 
    sem_post(&s); // signal here, child is done
    return NULL;
}

int main(int argc, char* argv[]) {
    sem_init(&s, 0, 0); // What should X be? - 0. Because main need to go to sleep after calling this.
    printf("parent: begin\n");
    pthread_t c;
    Pthread_create(&c, NULL, child, NULL);
    sem_wait(&s); //wait here for child, wait does this replace pthread_join?
    printf("parent: end");
    return 0;
}

// Seems like different initializations of semaphores are leading to different kinds of concurrency primitives

// init == 1 -> mutual exclusion primitive. like mutex locks
// init == 0 -> ordering primitive, like condition variables

