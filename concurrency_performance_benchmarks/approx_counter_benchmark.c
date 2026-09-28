#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "common_threads.h"

// ---- Approximate (sloppy) counter ----

// Cache line padding to avoid false sharing.
// Without this, adjacent local counters (local[0], local[1], ...) sit on
// the same 64-byte cache line. When 12 threads write to their own local[i]
// simultaneously, they bounce that single cache line between all cores —
// destroying the per-CPU locality that makes the approximate counter fast.
#define CACHE_LINE 64

typedef struct {
    int value;
    char _pad[CACHE_LINE - sizeof(int)];
} __attribute__((aligned(CACHE_LINE))) padded_int_t;

typedef struct {
    pthread_mutex_t lock;
    char _pad[CACHE_LINE - sizeof(pthread_mutex_t)];
} __attribute__((aligned(CACHE_LINE))) padded_lock_t;

typedef struct {
    int             global;
    pthread_mutex_t glock;
    padded_int_t   *local;      // per-CPU local counts (cache-line padded)
    padded_lock_t  *llock;      // per-CPU locks (cache-line padded)
    int             threshold;  // S: transfer frequency
    int             num_cpus;
} approx_counter_t;

void approx_init(approx_counter_t *c, int num_cpus, int threshold) {
    c->num_cpus = num_cpus;
    c->threshold = threshold;
    c->global = 0;
    Pthread_mutex_init(&c->glock, NULL);
    c->local = calloc(num_cpus, sizeof(padded_int_t));
    c->llock = malloc(num_cpus * sizeof(padded_lock_t));
    int i;
    for (i = 0; i < num_cpus; i++) {
        Pthread_mutex_init(&c->llock[i].lock, NULL);
    }
}

void approx_update(approx_counter_t *c, int threadID, int amt) {
    int cpu = threadID % c->num_cpus;
    Pthread_mutex_lock(&c->llock[cpu].lock);
    c->local[cpu].value += amt;
    if (c->local[cpu].value >= c->threshold) {
        Pthread_mutex_lock(&c->glock);
        c->global += c->local[cpu].value;
        Pthread_mutex_unlock(&c->glock);
        c->local[cpu].value = 0;
    }
    Pthread_mutex_unlock(&c->llock[cpu].lock);
}

// Drain remaining local counts into global (call after all threads joined)
int approx_drain_and_get(approx_counter_t *c) {
    int i;
    for (i = 0; i < c->num_cpus; i++) {
        c->global += c->local[i].value;
        c->local[i].value = 0;
    }
    return c->global;
}

void approx_destroy(approx_counter_t *c) {
    free(c->local);
    free(c->llock);
}

// ---- Thread worker ----

typedef struct {
    approx_counter_t *counter;
    int increments;
    int thread_id;
} thread_arg_t;

void *worker(void *arg) {
    thread_arg_t *a = (thread_arg_t *)arg;
    int i;
    for (i = 0; i < a->increments; i++) {
        approx_update(a->counter, a->thread_id, 1);
    }
    return NULL;
}

// ---- Benchmark runner ----

#define TOTAL_INCREMENTS (10000000)

void run_benchmark(int num_threads, int threshold, int num_cpus, double *out_ms) {
    approx_counter_t counter;
    approx_init(&counter, num_cpus, threshold);

    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    thread_arg_t *args = malloc(sizeof(thread_arg_t) * num_threads);

    int per_thread = TOTAL_INCREMENTS / num_threads;
    int i;

    for (i = 0; i < num_threads; i++) {
        args[i].counter = &counter;
        args[i].increments = per_thread;
        args[i].thread_id = i;
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

    // Correctness check: drain locals and verify
    int final_val = approx_drain_and_get(&counter);
    if (final_val != TOTAL_INCREMENTS) {
        fprintf(stderr, "ERROR: expected %d, got %d (threads=%d, S=%d)\n",
                TOTAL_INCREMENTS, final_val, num_threads, threshold);
    }

    free(threads);
    free(args);
    approx_destroy(&counter);
}

// ---- Thread count list builder (same logic as counter_benchmark.c) ----

int build_thread_counts(int num_cpus, int *thread_counts) {
    int fixed[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
    int num_fixed = sizeof(fixed) / sizeof(fixed[0]);
    int n = 0, cpu_inserted = 0, i;

    for (i = 0; i < num_fixed; i++) {
        if (!cpu_inserted && fixed[i] > num_cpus) {
            if (i == 0 || fixed[i - 1] != num_cpus)
                thread_counts[n++] = num_cpus;
            cpu_inserted = 1;
        }
        if (fixed[i] == num_cpus) cpu_inserted = 1;
        thread_counts[n++] = fixed[i];
    }
    if (!cpu_inserted) thread_counts[n++] = num_cpus;
    return n;
}

// ---- Main ----

int main(int argc, char *argv[]) {
    char *csv_dir = NULL;
    if (argc > 2 && strcmp(argv[1], "--csv") == 0) {
        csv_dir = argv[2];
    }

    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);

    // --- Sweep A: vary threads, for key threshold values ---
    int thresholds_a[] = {1, 10, 64, 512, 1024};
    int num_thresholds_a = sizeof(thresholds_a) / sizeof(thresholds_a[0]);

    int thread_counts[16];
    int num_thread_counts = build_thread_counts(num_cpus, thread_counts);

    // Storage for all results
    double results_a[5][16]; // [threshold_idx][thread_idx]
    int t, s;

    printf("CPUs available: %d\n", num_cpus);
    printf("Total increments per run: %d (constant, split across threads)\n", TOTAL_INCREMENTS);
    printf("Timer: clock_gettime(CLOCK_MONOTONIC)\n\n");

    printf("=== Sweep A: Vary threads, for each threshold S ===\n\n");

    for (s = 0; s < num_thresholds_a; s++) {
        int S = thresholds_a[s];
        printf("--- Threshold S = %d ---\n", S);
        printf("%-10s | %-12s | %-18s\n", "Threads", "Time (ms)", "Per-increment (ns)");
        printf("%-10s-+-%-12s-+-%-18s\n", "----------", "------------", "------------------");

        for (t = 0; t < num_thread_counts; t++) {
            run_benchmark(thread_counts[t], S, num_cpus, &results_a[s][t]);
            double per_inc = (results_a[s][t] * 1000000.0) / TOTAL_INCREMENTS;
            printf("%-10d | %-12.3f | %-18.2f\n", thread_counts[t], results_a[s][t], per_inc);
        }
        printf("\n");
    }

    // --- Sweep B: vary threshold, fixed thread count = num_cpus ---
    int thresholds_b[] = {1, 2, 5, 10, 32, 64, 128, 256, 512, 1024};
    int num_thresholds_b = sizeof(thresholds_b) / sizeof(thresholds_b[0]);
    double results_b[16]; // one result per threshold

    printf("=== Sweep B: Vary threshold S, fixed threads = %d (num_cpus) ===\n\n", num_cpus);
    printf("%-12s | %-12s | %-18s\n", "Threshold S", "Time (ms)", "Per-increment (ns)");
    printf("%-12s-+-%-12s-+-%-18s\n", "------------", "------------", "------------------");

    for (s = 0; s < num_thresholds_b; s++) {
        run_benchmark(num_cpus, thresholds_b[s], num_cpus, &results_b[s]);
        double per_inc = (results_b[s] * 1000000.0) / TOTAL_INCREMENTS;
        printf("%-12d | %-12.3f | %-18.2f\n", thresholds_b[s], results_b[s], per_inc);
    }

    // --- CSV output ---
    if (csv_dir) {
        char path[512];

        // Sweep A: one CSV per threshold (compatible with plot_results.py)
        for (s = 0; s < num_thresholds_a; s++) {
            int S = thresholds_a[s];
            snprintf(path, sizeof(path), "%s/approx_S%d_results.csv", csv_dir, S);
            FILE *fp = fopen(path, "w");
            if (!fp) { perror(path); continue; }
            fprintf(fp, "threads,time_ms,per_increment_ns\n");
            for (t = 0; t < num_thread_counts; t++) {
                double per_inc = (results_a[s][t] * 1000000.0) / TOTAL_INCREMENTS;
                fprintf(fp, "%d,%.3f,%.2f\n", thread_counts[t], results_a[s][t], per_inc);
            }
            fclose(fp);
        }

        // Sweep B: threshold sweep CSV
        snprintf(path, sizeof(path), "%s/approx_by_threshold_results.csv", csv_dir);
        FILE *fp = fopen(path, "w");
        if (fp) {
            fprintf(fp, "threshold,time_ms,per_increment_ns\n");
            for (s = 0; s < num_thresholds_b; s++) {
                double per_inc = (results_b[s] * 1000000.0) / TOTAL_INCREMENTS;
                fprintf(fp, "%d,%.3f,%.2f\n", thresholds_b[s], results_b[s], per_inc);
            }
            fclose(fp);
        }

        printf("\nCSVs saved to: %s/\n", csv_dir);
    }

    return 0;
}
