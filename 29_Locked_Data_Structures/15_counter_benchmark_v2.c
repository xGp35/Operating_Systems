#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>       // clock_gettime
#include <unistd.h>     // sysconf(_SC_NPROCESSORS_ONLN)
#include "common_threads.h"

// ---- Concurrent counter (from 02_counter_with_locks.c) ----

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
    int increments;
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

#define TOTAL_INCREMENTS (10000000) // 10 million — constant total work

// Measures wall-clock time: the real-world time from first thread creation to last thread join.
// This includes: actual computation + lock contention (waiting) + context switching + OS scheduling.
// It is NOT per-thread CPU time. It's "how long did the whole operation take?"

void run_benchmark(int num_threads, double *out_ms) {
    counter_t counter;
    counter_init(&counter);

    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    thread_arg_t *args = malloc(sizeof(thread_arg_t) * num_threads);

    int per_thread = TOTAL_INCREMENTS / num_threads;
    int i;

    for (i = 0; i < num_threads; i++) {
        args[i].counter = &counter;
        args[i].increments = per_thread;
    }
    // Last thread picks up remainder so total is exactly TOTAL_INCREMENTS
    args[num_threads - 1].increments += TOTAL_INCREMENTS - (per_thread * num_threads);

    // Time it
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (i = 0; i < num_threads; i++) {
        Pthread_create(&threads[i], NULL, worker, &args[i]);
    }
    for (i = 0; i < num_threads; i++) {
        Pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);
    *out_ms = elapsed_ns / 1000000.0;

    // Verify correctness
    int final_val = counter_get(&counter);
    if (final_val != TOTAL_INCREMENTS) {
        printf("ERROR: expected %d, got %d (threads=%d)\n", TOTAL_INCREMENTS, final_val, num_threads);
    }

    free(threads);
    free(args);
}

// ---- Main ----

int main() {
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    printf("CPUs available: %d\n", num_cpus);
    printf("Total increments per run: %d (constant, split across threads)\n", TOTAL_INCREMENTS);
    printf("Timer: clock_gettime(CLOCK_MONOTONIC)\n");
    printf("Time reported: wall-clock time (thread creation → last thread joined)\n\n");

    // Thread counts: powers of 2, then 64-increments in 128-256, then big jumps
    int fixed_counts[] = {1, 2, 4, 8, 16, 32, 64, 128, 192, 256, 512, 1024};
    int num_fixed = sizeof(fixed_counts) / sizeof(fixed_counts[0]);

    // Build final list, inserting CPU count in sorted position if not already present
    int thread_counts[16]; // enough room for fixed_counts + 1 extra
    int num_tests = 0;
    int cpu_inserted = 0;
    int i;

    for (i = 0; i < num_fixed; i++) {
        // Insert CPU count right before the first entry that exceeds it
        if (!cpu_inserted && fixed_counts[i] > num_cpus) {
            // Only insert if it's not equal to the previous entry
            if (i == 0 || fixed_counts[i - 1] != num_cpus) {
                thread_counts[num_tests++] = num_cpus;
            }
            cpu_inserted = 1;
        }
        // Skip if this fixed count equals the CPU count we just inserted
        if (fixed_counts[i] == num_cpus) {
            cpu_inserted = 1;
        }
        thread_counts[num_tests++] = fixed_counts[i];
    }
    // If CPU count is larger than all fixed counts, append it
    if (!cpu_inserted) {
        thread_counts[num_tests++] = num_cpus;
    }

    // Print table
    printf("%-10s | %-12s | %-18s\n", "Threads", "Time (ms)", "Per-increment (ns)");
    printf("%-10s-+-%-12s-+-%-18s\n", "----------", "------------", "------------------");

    double ms;
    for (i = 0; i < num_tests; i++) {
        run_benchmark(thread_counts[i], &ms);
        double per_inc_ns = (ms * 1000000.0) / TOTAL_INCREMENTS; // ms → ns, then divide by ops
        printf("%-10d | %-12.3f | %-18.2f\n", thread_counts[i], ms, per_inc_ns);
    }

    printf("\n");
    printf("What to look for:\n");
    printf("  - 1 thread: no contention. Baseline cost of lock + increment.\n");
    printf("  - 2+ threads: time increases due to mutex contention.\n");
    printf("  - ~%d threads (CPU count): transition point. Beyond this,\n", num_cpus);
    printf("    threads also compete for CPU time, not just the lock.\n");
    printf("  - Per-increment (ns) shows the average cost of one locked increment.\n");
    printf("    With 1 thread this is just lock+unlock+increment overhead.\n");
    printf("    With many threads it includes time spent WAITING for the lock.\n");

    return 0;
}

// Compile and run:
//   gcc -o bench_v2 15_counter_benchmark_v2.c -lpthread
//   ./bench_v2
