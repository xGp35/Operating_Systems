#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <time.h>

#define ITERATIONS 100000

long long now_ns() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000000000LL + t.tv_nsec;
}

void set_affinity(int tag) {
    thread_affinity_policy_data_t policy = { tag };

    kern_return_t result = thread_policy_set(
        mach_thread_self(),
        THREAD_AFFINITY_POLICY,
        (thread_policy_t)&policy,
        THREAD_AFFINITY_POLICY_COUNT
    );

    if (result != KERN_SUCCESS) {
        fprintf(stderr, "thread_policy_set failed: %d\n", result);
        exit(1);
    }
}

int main() {
    int pipe1[2];
    int pipe2[2];

    if (pipe(pipe1) || pipe(pipe2)) {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    char buffer = 'x';

    if (pid == 0) {
        // CHILD
        close(pipe1[1]);
        close(pipe2[0]);

        // Same affinity tag as parent.
        set_affinity(1);

        for (int i = 0; i < ITERATIONS; i++) {
            read(pipe1[0], &buffer, 1);
            write(pipe2[1], &buffer, 1);
        }

        exit(0);
    }

    // PARENT
    close(pipe1[0]);
    close(pipe2[1]);

    // Same affinity tag as child.
    set_affinity(1);

    long long start = now_ns();

    for (int i = 0; i < ITERATIONS; i++) {
        write(pipe1[1], &buffer, 1);
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