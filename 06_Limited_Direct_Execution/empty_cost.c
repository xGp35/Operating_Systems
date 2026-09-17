#include <stdio.h>
#include <time.h>

#define ITERATIONS 10000000

int main() {
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (long i = 0; i < ITERATIONS; i++) {
        // nothing
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    long long elapsed_ns =
        (end.tv_sec - start.tv_sec) * 1000000000LL +
        (end.tv_nsec - start.tv_nsec);

    printf("Total: %lld ns\n", elapsed_ns);
    printf("Per iteration: %.2f ns\n",
           (double)elapsed_ns / ITERATIONS);

    return 0;
}