#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#include "common.h"
#include "common_threads.h"

static volatile int counter = 0;

// mythread() 
// 
// Simply adds 1 to counter repeatedly, in a loop
// No, this is not how to a add 10,000,000 to a counter,
// but it shows the problem nicely

void *mythread(void *arg) {
    printf("%s: begin \n", (char *) arg);
    int i;
    for (i=0; i < 1e5; i++) {
        counter = counter + 1;
    }
    printf("%s: done\n", (char *) arg);
    return NULL; // Why return NULL instead of 0 or 1?
}

// main()
// Just launches two threads (pthread_create) and then waits for them (pthread_join)

int main(int argc, char *argv[]){
    pthread_t p1, p2;
    printf("main: begin (counter = %d)\n", counter);
    Pthread_create(&p1, NULL, mythread, "A");
    Pthread_create(&p2, NULL, mythread, "B");

    //join waits for the threads to finish
    Pthread_join(p1, NULL); // why is the second parameter NULL, what would have happened if I hadn't added this line?
    // 2nd parameter gives-> where to store that thread’s return value (void **retval)
    Pthread_join(p2, NULL);
    printf("main: done with both (counter = %d)\n", counter);
    return 0;
}
/*
If you removed the whole Pthread_join line
If you didn’t call Pthread_join at all:

main could reach return 0; and exit before the threads finish.
When the process exits, all threads are killed.
So you’d often see counter much less than the expected value, because the worker threads might still be in the middle of their loops when the process dies.
That’s why you must keep the Pthread_join calls (to wait for the threads), and you pass NULL because you don’t care about their return values.
*/