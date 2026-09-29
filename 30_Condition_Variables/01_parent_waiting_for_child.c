#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "common_threads.h"
// THE HOW? - Note - This wion't compile, just for my learning

void *child(void *arg) {
    printf("child\n");
    // XXX how to indicate we are done.
    return NULL;
}

int main (int argc, char *argv[]) {
    printf("parent: begin\n");
    pthread_t c;
    Pthread_create(&c, NULL, child, NULL); //child
    // XXX how to wait for child
    printf("parent: end\n");
    return 0;
}

//Approach 1 - Spin Waiting
volatile int done = 0;

void* child(void* arg) {
    printf("child\n");
    done = 1;
    return NULL;
}

int main (int argc, char* argv[]) {
    printf("parent: begin\n");
    pthread_t c;
    Pthread_create(&c, NULL, child, NULL);
    while (done == 0) {
        ; //spin
    }
    printf("parent: end\n");
    return 0;
}
