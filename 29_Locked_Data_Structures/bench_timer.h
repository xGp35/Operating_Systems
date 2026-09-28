#ifndef __bench_timer_h__
#define __bench_timer_h__

// bench_timer.h — Reusable header-only timer utility
//
// Wraps all three timing methods behind a simple API:
//   timer_start(&t)   — capture start time using all available methods
//   timer_stop(&t)    — capture end time
//   timer_print(&t, label)  — print elapsed time from all methods
//
// Auto-detects platform:
//   Always:        gettimeofday (usec), clock_gettime (nsec)
//   Linux x86_64:  + RDTSC (cycles)
//   macOS ARM:     + mach_absolute_time (ticks → ns)
//
// Usage:
//   #include "bench_timer.h"
//   bench_timer_t t;
//   timer_start(&t);
//   // ... code to measure ...
//   timer_stop(&t);
//   timer_print(&t, "my operation");

#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

// --- Platform-specific includes and helpers ---

#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif

// RDTSC: only on x86_64
#if defined(__x86_64__)

static inline uint64_t _bench_rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static inline uint64_t _bench_rdtscp(void) {
    uint32_t lo, hi, aux;
    __asm__ __volatile__ ("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux));
    return ((uint64_t)hi << 32) | lo;
}

#endif // __x86_64__

// mach_absolute_time: only on macOS
#if defined(__APPLE__)

static inline uint64_t _bench_mach_time(void) {
    return mach_absolute_time();
}

// Returns the timebase ratio (call once, cache the result)
static inline mach_timebase_info_data_t _bench_mach_timebase(void) {
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    return info;
}

#endif // __APPLE__

// --- The timer struct ---

typedef struct {
    // gettimeofday
    struct timeval gtod_start;
    struct timeval gtod_end;

    // clock_gettime
    struct timespec cgt_start;
    struct timespec cgt_end;

#if defined(__x86_64__)
    // RDTSC
    uint64_t rdtsc_start;
    uint64_t rdtsc_end;
#endif

#if defined(__APPLE__)
    // mach_absolute_time
    uint64_t mach_start;
    uint64_t mach_end;
    mach_timebase_info_data_t mach_timebase;
#endif
} bench_timer_t;

// --- API ---

static inline void timer_start(bench_timer_t *t) {
#if defined(__APPLE__)
    t->mach_timebase = _bench_mach_timebase();
#endif

    // Start all timers as close together as possible
#if defined(__x86_64__)
    t->rdtsc_start = _bench_rdtsc();
#endif
#if defined(__APPLE__)
    t->mach_start = _bench_mach_time();
#endif
    clock_gettime(CLOCK_MONOTONIC, &t->cgt_start);
    gettimeofday(&t->gtod_start, NULL);
}

static inline void timer_stop(bench_timer_t *t) {
    // Stop all timers as close together as possible (reverse order for tighter measurement)
    gettimeofday(&t->gtod_end, NULL);
    clock_gettime(CLOCK_MONOTONIC, &t->cgt_end);
#if defined(__APPLE__)
    t->mach_end = _bench_mach_time();
#endif
#if defined(__x86_64__)
    t->rdtsc_end = _bench_rdtscp(); // serializing version for accurate end measurement
#endif
}

// Get elapsed time in milliseconds (from clock_gettime — most reliable)
static inline double timer_elapsed_ms(bench_timer_t *t) {
    long ns = (t->cgt_end.tv_sec - t->cgt_start.tv_sec) * 1000000000L
            + (t->cgt_end.tv_nsec - t->cgt_start.tv_nsec);
    return ns / 1000000.0;
}

// Get elapsed time in microseconds (from gettimeofday)
static inline long timer_elapsed_usec(bench_timer_t *t) {
    return (t->gtod_end.tv_sec - t->gtod_start.tv_sec) * 1000000
         + (t->gtod_end.tv_usec - t->gtod_start.tv_usec);
}

// Get elapsed time in nanoseconds (from clock_gettime)
static inline long timer_elapsed_ns(bench_timer_t *t) {
    return (t->cgt_end.tv_sec - t->cgt_start.tv_sec) * 1000000000L
         + (t->cgt_end.tv_nsec - t->cgt_start.tv_nsec);
}

#if defined(__x86_64__)
// Get elapsed CPU cycles (from RDTSC)
static inline uint64_t timer_elapsed_cycles(bench_timer_t *t) {
    return t->rdtsc_end - t->rdtsc_start;
}
#endif

#if defined(__APPLE__)
// Get elapsed time in nanoseconds (from mach_absolute_time)
static inline double timer_elapsed_mach_ns(bench_timer_t *t) {
    uint64_t ticks = t->mach_end - t->mach_start;
    return (double)(ticks * t->mach_timebase.numer) / t->mach_timebase.denom;
}
#endif

// Print all available timer results on one line
static inline void timer_print(bench_timer_t *t, const char *label) {
    long usec = timer_elapsed_usec(t);
    long ns = timer_elapsed_ns(t);
    double ms = timer_elapsed_ms(t);

    printf("%-30s | gtod: %8ld usec | cgt: %12ld ns (%.3f ms)", label, usec, ns, ms);

#if defined(__x86_64__)
    printf(" | rdtsc: %12llu cycles", (unsigned long long)timer_elapsed_cycles(t));
#endif

#if defined(__APPLE__)
    printf(" | mach: %.0f ns (%.3f ms)", timer_elapsed_mach_ns(t), timer_elapsed_mach_ns(t) / 1000000.0);
#endif

    printf("\n");
}

// Print a compact table header matching timer_print_row format
static inline void timer_print_table_header(void) {
    printf("%-10s | %-14s | %-14s", "Threads", "gtod (ms)", "cgt (ms)");
#if defined(__x86_64__)
    printf(" | %-14s", "rdtsc (cycles)");
#endif
#if defined(__APPLE__)
    printf(" | %-14s", "mach (ms)");
#endif
    printf("\n");

    printf("%-10s-+-%-14s-+-%-14s", "----------", "--------------", "--------------");
#if defined(__x86_64__)
    printf("-+-%-14s", "--------------");
#endif
#if defined(__APPLE__)
    printf("-+-%-14s", "--------------");
#endif
    printf("\n");
}

// Print one row of a benchmark table (compact format for tabular output)
static inline void timer_print_row(bench_timer_t *t, int threads) {
    double gtod_ms = timer_elapsed_usec(t) / 1000.0;
    double cgt_ms = timer_elapsed_ms(t);

    printf("%-10d | %-14.3f | %-14.3f", threads, gtod_ms, cgt_ms);

#if defined(__x86_64__)
    printf(" | %-14llu", (unsigned long long)timer_elapsed_cycles(t));
#endif

#if defined(__APPLE__)
    printf(" | %-14.3f", timer_elapsed_mach_ns(t) / 1000000.0);
#endif

    printf("\n");
}

#endif // __bench_timer_h__
