#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "common.h"
#include "common_threads.h"
//void *mythread means the return type of this function is a pointer, of no kind so it can be casted into any type that we want
//void *arg means the function takes a pointer as its argument, So arg is a pointer to void,
void *mythread(void *arg) {
    printf("%s\n", (char *) arg); //
}

int main(int argc, char *argv[]) {
    pthread_t p1, p2;
    int rc;
    printf("main: begin\n");
    // pthread_create() -> Create a new thread and tell it what function to run
    Pthread_create(&p1, NULL, mythread, "A"); // &p1 -> where to store the thread ID, NULL -> Thread attributes, mythread -> function the thread should run, "A" -> argument to mythread
    Pthread_create(&p2, NULL, mythread, "B");
    // join waits for the threads to finish
    Pthread_join(p1, NULL);
    Pthread_join(p2, NULL);
    printf("main: end\n");
    return 0;
}