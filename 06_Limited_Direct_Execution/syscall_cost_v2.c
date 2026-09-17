#include <stdio.h>
#include <unistd.h>
#include <time.h>

#define ITERATIONS 10000000

long long now_ns() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000000000LL + t.tv_nsec;
}

int main() {
    char buffer[1];

    // -------------------------
    // Experiment 1: CONTROL
    // -------------------------
    long long start = now_ns();

    for (long i = 0; i < ITERATIONS; i++) {
        // Nothing happens here.
        // This measures the cost of the loop itself.
    }

    long long end = now_ns();

    long long control = end - start;


    // -------------------------
    // Experiment 2: SYSCALL
    // -------------------------
    start = now_ns();

    for (long i = 0; i < ITERATIONS; i++) {
        // Enter the kernel and immediately come back.
        read(STDIN_FILENO, buffer, 0);
    }

    end = now_ns();

    long long syscall = end - start;


    // -------------------------
    // RESULTS
    // -------------------------

    printf("Control total:     %lld ns\n", control);
    printf("Syscall total:     %lld ns\n", syscall);

    printf("Control / iteration: %.2f ns\n",
           (double)control / ITERATIONS);

    printf("Syscall / iteration: %.2f ns\n",
           (double)syscall / ITERATIONS);

    printf("Estimated syscall cost: %.2f ns\n",
           (double)(syscall - control) / ITERATIONS);

    return 0;
}