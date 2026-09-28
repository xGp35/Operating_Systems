#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>     // sysconf(_SC_NPROCESSORS_ONLN)
#include "common_threads.h"
#include "bench_timer.h"

// ---- Concurrent counter (from 02_counter_with_locks.c) ----
// Copied here so this file is self-contained. No need for a separate header.

typedef struct __counter_t {
    int value;
    pthread_mutex_t mutex;
} counter_t;

void counter_init(counter_t *c) {
    c->value = 0;
    Pthread_mutex_init(&c->mutex, NULL);
}

void counter_increment(counter_t *c) {
    Pthread_mutex_lock(&c->mutex);
    c->value++;
    Pthread_mutex_unlock(&c->mutex);
}

int counter_get(counter_t *c) {
    Pthread_mutex_lock(&c->mutex);
    int rc = c->value;
    Pthread_mutex_unlock(&c->mutex);
    return rc;
}

// ---- Thread worker ----

typedef struct {
    counter_t *counter;
    int increments;     // how many times this thread should increment
} thread_arg_t;

void *worker(void *arg) {
    thread_arg_t *a = (thread_arg_t *)arg;
    int i;
    for (i = 0; i < a->increments; i++) {
        counter_increment(a->counter);
    }
    return NULL;
}

// ---- Benchmark runner ----
// Runs the counter benchmark with `num_threads` threads.
// Total work is constant: TOTAL_INCREMENTS split evenly across threads.
// Returns the timer result via the `result` pointer.

#define TOTAL_INCREMENTS (10000000) // 10 million — same total work regardless of thread count

void run_benchmark(int num_threads, bench_timer_t *result) {
    counter_t counter;
    counter_init(&counter);

    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    thread_arg_t *args = malloc(sizeof(thread_arg_t) * num_threads);

    int per_thread = TOTAL_INCREMENTS / num_threads;
    int i;

    // Set up args
    for (i = 0; i < num_threads; i++) {
        args[i].counter = &counter;
        args[i].increments = per_thread;
    }
    // Give the last thread any remainder (so total is exactly TOTAL_INCREMENTS)
    args[num_threads - 1].increments += TOTAL_INCREMENTS - (per_thread * num_threads);

    // Time the threaded work
    timer_start(result);

    for (i = 0; i < num_threads; i++) {
        Pthread_create(&threads[i], NULL, worker, &args[i]);
    }
    for (i = 0; i < num_threads; i++) {
        Pthread_join(threads[i], NULL);
    }

    timer_stop(result);

    // Verify correctness
    int final_val = counter_get(&counter);
    if (final_val != TOTAL_INCREMENTS) {
        printf("ERROR: expected %d, got %d\n", TOTAL_INCREMENTS, final_val);
    }

    free(threads);
    free(args);
}

// ---- Main ----

int main() {
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    printf("CPUs available: %d\n", num_cpus);
    printf("Total increments per run: %d (constant, split across threads)\n\n", TOTAL_INCREMENTS);

    // Thread counts to test: 1, 2, 4, 8, ... up to 2x CPUs
    // Also include the exact CPU count if it's not a power of 2
    int thread_counts[16];
    int num_tests = 0;

    int t = 1;
    while (t <= num_cpus * 2) {
        thread_counts[num_tests++] = t;
        // Insert exact CPU count if we'd skip over it
        if (t < num_cpus && t * 2 > num_cpus && num_cpus != t) {
            thread_counts[num_tests++] = num_cpus;
        }
        t *= 2;
    }

    // Print table
    timer_print_table_header();

    bench_timer_t result;
    int i;
    for (i = 0; i < num_tests; i++) {
        run_benchmark(thread_counts[i], &result);
        timer_print_row(&result, thread_counts[i]);
    }

    printf("\n");
    printf("What to look for:\n");
    printf("  - With 1 thread, there's no lock contention. This is the baseline.\n");
    printf("  - As threads increase, time should INCREASE because all threads\n");
    printf("    fight over the same mutex. More threads = more contention.\n");
    printf("  - Beyond %d threads (your CPU count), it gets even worse because\n", num_cpus);
    printf("    threads are now context-switching on top of contending.\n");

    return 0;
}

// Compile and run:
//   gcc -o counter_bench 14_counter_benchmark.c -lpthread
//   ./counter_bench
