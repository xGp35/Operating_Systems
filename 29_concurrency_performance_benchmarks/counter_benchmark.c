#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "common_threads.h"

// ---- Concurrent counter (simple lock-based) ----

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

#define TOTAL_INCREMENTS (10000000)

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
    args[num_threads - 1].increments += TOTAL_INCREMENTS - (per_thread * num_threads);

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

    int final_val = counter_get(&counter);
    if (final_val != TOTAL_INCREMENTS) {
        fprintf(stderr, "ERROR: expected %d, got %d (threads=%d)\n",
                TOTAL_INCREMENTS, final_val, num_threads);
    }

    free(threads);
    free(args);
}

// ---- Main ----

int main(int argc, char *argv[]) {
    // --csv FILE: write CSV to FILE (in addition to the table on stdout)
    // Without --csv: just print the table
    char *csv_path = NULL;
    if (argc > 2 && strcmp(argv[1], "--csv") == 0) {
        csv_path = argv[2];
    }

    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);

    // Thread counts to test
    int fixed_counts[] = {1, 2, 4, 8, 16, 32, 64, 128, 192, 256, 512, 1024};
    int num_fixed = sizeof(fixed_counts) / sizeof(fixed_counts[0]);

    int thread_counts[16];
    int num_tests = 0;
    int cpu_inserted = 0;
    int i;

    for (i = 0; i < num_fixed; i++) {
        if (!cpu_inserted && fixed_counts[i] > num_cpus) {
            if (i == 0 || fixed_counts[i - 1] != num_cpus) {
                thread_counts[num_tests++] = num_cpus;
            }
            cpu_inserted = 1;
        }
        if (fixed_counts[i] == num_cpus) {
            cpu_inserted = 1;
        }
        thread_counts[num_tests++] = fixed_counts[i];
    }
    if (!cpu_inserted) {
        thread_counts[num_tests++] = num_cpus;
    }

    // Run all benchmarks once, store results
    double results_ms[16];
    for (i = 0; i < num_tests; i++) {
        run_benchmark(thread_counts[i], &results_ms[i]);
    }

    // Always print human-readable table to stdout
    printf("CPUs available: %d\n", num_cpus);
    printf("Total increments per run: %d (constant, split across threads)\n", TOTAL_INCREMENTS);
    printf("Timer: clock_gettime(CLOCK_MONOTONIC)\n");
    printf("Time reported: wall-clock time (thread creation -> last thread joined)\n\n");

    printf("%-10s | %-12s | %-18s\n", "Threads", "Time (ms)", "Per-increment (ns)");
    printf("%-10s-+-%-12s-+-%-18s\n", "----------", "------------", "------------------");

    for (i = 0; i < num_tests; i++) {
        double per_inc_ns = (results_ms[i] * 1000000.0) / TOTAL_INCREMENTS;
        printf("%-10d | %-12.3f | %-18.2f\n", thread_counts[i], results_ms[i], per_inc_ns);
    }

    printf("\n");
    printf("What to look for:\n");
    printf("  - 1 thread: no contention. Baseline cost of lock + increment.\n");
    printf("  - 2+ threads: time increases due to mutex contention.\n");
    printf("  - ~%d threads (CPU count): transition point.\n", num_cpus);
    printf("  - Per-increment (ns) shows avg cost including lock wait time.\n");

    // If --csv FILE was given, write the same data as CSV to that file
    if (csv_path) {
        FILE *fp = fopen(csv_path, "w");
        if (!fp) {
            perror("Failed to open CSV file");
            return 1;
        }
        fprintf(fp, "threads,time_ms,per_increment_ns\n");
        for (i = 0; i < num_tests; i++) {
            double per_inc_ns = (results_ms[i] * 1000000.0) / TOTAL_INCREMENTS;
            fprintf(fp, "%d,%.3f,%.2f\n", thread_counts[i], results_ms[i], per_inc_ns);
        }
        fclose(fp);
        printf("\nCSV saved to: %s\n", csv_path);
    }

    return 0;
}
