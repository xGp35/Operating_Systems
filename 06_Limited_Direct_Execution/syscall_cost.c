#include <stdio.h>
#include <unistd.h>     // Hey! I know about system calls like read()
#include <time.h>       // And I can keep track of time for you

#define ITERATIONS 10000000  // We're going to make read() do this 10 million times!

int main() {
    char buffer[1];  // Don't worry, read(). We're giving you a tiny 1-byte home.

    struct timespec start, end;  // These two will watch the clock for us.

    // "Alright, clock, remember exactly what time we started!"
    clock_gettime(CLOCK_MONOTONIC, &start);

    // "Okay read(), your big moment has arrived.
    //  Please make a system call 10 million times!"
    for (long i = 0; i < ITERATIONS; i++) {

        // read() says:
        // "I'll go into the kernel and come right back...
        // but don't actually give me any data (0 bytes), okay?"
        read(STDIN_FILENO, buffer, 0);
    }

    // "Clock, tell me what time it is now!"
    clock_gettime(CLOCK_MONOTONIC, &end);

    // "Let's see how much time passed between the two clock readings."
    long long elapsed_ns =
        (end.tv_sec - start.tv_sec) * 1000000000LL +
        (end.tv_nsec - start.tv_nsec);

    // "Here's how long all 10 million trips into the kernel took."
    printf("Total time: %lld ns\n", elapsed_ns);

    // "And here's how many trips we made."
    printf("Iterations: %d\n", ITERATIONS);

    // "Divide the total time by the number of trips
    // to estimate the cost of ONE system call."
    printf("Average syscall cost: %.2f ns\n",
           (double)elapsed_ns / ITERATIONS);

    return 0;  // "My work here is done. Bye!"
}