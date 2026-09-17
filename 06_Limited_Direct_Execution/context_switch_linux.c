#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>
#include <time.h>

#define ITERATIONS 100000

long long now_ns() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000000000LL + t.tv_nsec;
}

int main() {
    int pipe1[2];
    int pipe2[2];

    pipe(pipe1);
    pipe(pipe2);

    // Pin this process to CPU 0.
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);

    if (sched_setaffinity(0, sizeof(set), &set) != 0) {
        perror("sched_setaffinity");
        exit(1);
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    char buffer = 'x';

    if (pid == 0) {
        // =========================
        // CHILD
        // =========================

        // fork() inherited the CPU affinity,
        // so the child is also pinned to CPU 0.

        close(pipe1[1]);
        close(pipe2[0]);

        for (int i = 0; i < ITERATIONS; i++) {

            // Wait for parent.
            // Child blocks → scheduler can run parent.
            read(pipe1[0], &buffer, 1);

            // Wake parent.
            write(pipe2[1], &buffer, 1);
        }

        exit(0);
    }

    // =========================
    // PARENT
    // =========================

    close(pipe1[0]);
    close(pipe2[1]);

    long long start = now_ns();

    for (int i = 0; i < ITERATIONS; i++) {

        // Wake child.
        write(pipe1[1], &buffer, 1);

        // Parent blocks waiting for child.
        // Scheduler switches to child.
        read(pipe2[0], &buffer, 1);
    }

    long long end = now_ns();

    wait(NULL);

    long long total_ns = end - start;

    printf("Total time: %lld ns\n", total_ns);
    printf("Iterations: %d\n", ITERATIONS);

    printf("Round-trip: %.2f ns\n",
           (double)total_ns / ITERATIONS);

    printf("Estimated context switch: %.2f ns\n",
           (double)total_ns / (2 * ITERATIONS));

    return 0;
}