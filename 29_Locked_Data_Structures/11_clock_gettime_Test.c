#include <stdio.h>
#include <stdlib.h>
#include <time.h>   // clock_gettime lives here, not sys/time.h

// clock_gettime() fills a struct timespec with the current time.
// struct timespec has two fields:
//   tv_sec  — seconds
//   tv_nsec — nanoseconds (the fractional part, 0 to 999999999)
// So the resolution is nanoseconds (1 billionth of a second) — 1000x finer than gettimeofday.

// Why CLOCK_MONOTONIC instead of CLOCK_REALTIME?
//   CLOCK_REALTIME = wall clock. Can jump backwards if someone adjusts the system time (NTP sync, manual change).
//   CLOCK_MONOTONIC = only goes forward. Immune to clock adjustments. Always use this for measuring durations.
//   Think of it as: REALTIME = "what time is it?", MONOTONIC = "how long has it been?"

int main() {
    struct timespec start, end;

    // --- Example 1: Measure a simple loop ---
    clock_gettime(CLOCK_MONOTONIC, &start);

    int i;
    volatile int sum = 0; // volatile so the compiler doesn't optimize this away
    for (i = 0; i < 1000000; i++) {
        sum += i;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculate elapsed time in nanoseconds
    // Same idea as gettimeofday, but tv_nsec wraps at 1000000000 (1 billion = 1 second)
    long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);

    printf("Loop of 1,000,000 additions took %ld nanoseconds (%.3f ms)\n",
           elapsed_ns, elapsed_ns / 1000000.0);

    // --- Example 2: How precise is clock_gettime itself? ---
    // Call it twice back-to-back. This should now show a non-zero value
    // unlike gettimeofday which showed 0 (because its resolution was only microseconds).
    clock_gettime(CLOCK_MONOTONIC, &start);
    clock_gettime(CLOCK_MONOTONIC, &end);

    long overhead = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);
    printf("Two back-to-back clock_gettime() calls: %ld nanoseconds apart\n", overhead);
    // This tells you the actual cost of the timing call itself.
    // On most machines this is ~20-80 nanoseconds.

    // --- Example 3: Measure something tiny — a single operation ---
    int iterations = 10000000; // 10 million
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 0; i < iterations; i++) {
        sum += 1;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    long total_ns = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);
    double per_op_ns = (double)total_ns / iterations;
    printf("10M tiny ops took %ld ns total, %.2f nanoseconds per op\n", total_ns, per_op_ns);

    return 0;
}

// Compile and run:
//   gcc -o clock_gettime_test 11_clock_gettime_Test.c
//   ./clock_gettime_test
//
// Comparison with gettimeofday:
//   gettimeofday  → struct timeval  → tv_sec + tv_usec  → microsecond resolution (10^-6)
//   clock_gettime → struct timespec → tv_sec + tv_nsec  → nanosecond resolution  (10^-9)
//
// gettimeofday is older (POSIX.1-2001) and technically marked obsolescent.
// clock_gettime is the modern replacement. Prefer it for new code.
