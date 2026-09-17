#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
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

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    // =========================
    // CHILD PROCESS
    // =========================
    if (pid == 0) {

        char buffer;

        // Child doesn't need these ends.
        close(pipe1[1]);
        close(pipe2[0]);

        for (int i = 0; i < ITERATIONS; i++) {

            // Wait for parent.
            // This blocks until the parent writes.
            read(pipe1[0], &buffer, 1);

            // Wake parent up.
            write(pipe2[1], &buffer, 1);
        }

        exit(0);
    }

    // =========================
    // PARENT PROCESS
    // =========================

    char buffer = 'x';

    close(pipe1[0]);
    close(pipe2[1]);

    long long start = now_ns();

    for (int i = 0; i < ITERATIONS; i++) {

        // Wake child up.
        write(pipe1[1], &buffer, 1);

        // Wait for child to wake us up.
        // This causes the parent to block,
        // allowing the child to run.
        read(pipe2[0], &buffer, 1);
    }

    long long end = now_ns();

    wait(NULL);

    long long total_ns = end - start;

    printf("Total time: %lld ns\n", total_ns);
    printf("Iterations: %d\n", ITERATIONS);

    printf("Round-trip cost: %.2f ns\n",
           (double)total_ns / ITERATIONS);

    printf("Estimated context switch cost: %.2f ns\n",
           (double)total_ns / (2 * ITERATIONS));

    return 0;
}