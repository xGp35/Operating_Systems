#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>   // This is the header that gives us gettimeofday()

// gettimeofday() fills a struct timeval with the current time.
// struct timeval has two fields:
//   tv_sec  — seconds since Jan 1, 1970 (Unix epoch)
//   tv_usec — microseconds (the fractional part, 0 to 999999)
// So the resolution is microseconds (1 millionth of a second).
// That said, the ACTUAL accuracy depends on your hardware/OS — it might not be precise down to 1 usec.

// To measure how long something takes:
//   1. Call gettimeofday() before the thing   → start
//   2. Call gettimeofday() after the thing    → end
//   3. Subtract: elapsed = end - start

int main() {
    struct timeval start, end;

    // --- Example 1: Measure a simple loop ---
    gettimeofday(&start, NULL); // NULL because we don't care about timezone info (second arg)
    
    int i;
    volatile int sum = 0; // volatile so the compiler doesn't optimize this away
    for (i = 0; i < 1000000; i++) {
        sum += i;
    }
    
    gettimeofday(&end, NULL);

    // Calculate elapsed time in microseconds
    // Why this formula? Because tv_usec wraps around at 1000000 (1 second).
    // If start = 2.900000s and end = 3.100000s, then:
    //   end.tv_sec - start.tv_sec = 1, end.tv_usec - start.tv_usec = -800000
    //   So: (1 * 1000000) + (-800000) = 200000 usec = 0.2 seconds. Correct!
    long elapsed_usec = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);

    printf("Loop of 1,000,000 additions took %ld microseconds (%.3f ms)\n",
           elapsed_usec, elapsed_usec / 1000.0);
    /* on MacM4
    Loop of 1,000,000 additions took 2606 microseconds (2.606 ms)
    Two back-to-back gettimeofday() calls: 0 microseconds apart
    10M tiny ops took 11355 usec total, 1.14 nanoseconds per op*/

    // --- Example 2: How precise is gettimeofday itself? ---
    // Call it twice back-to-back with nothing in between.
    // The difference tells you the overhead of the call itself.
    gettimeofday(&start, NULL);
    gettimeofday(&end, NULL);

    long overhead = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    printf("Two back-to-back gettimeofday() calls: %ld microseconds apart\n", overhead);
    // If this prints 0, the call takes less than 1 microsecond — that's the resolution limit.
    // Anything you try to measure that takes less than ~1 usec won't show up with this timer.

    // --- Example 3: Measure something tiny — a single function call ---
    // To measure something very fast, run it many times and divide.
    int iterations = 10000000; // 10 million
    gettimeofday(&start, NULL);
    for (i = 0; i < iterations; i++) {
        sum += 1; // stand-in for whatever tiny operation you want to time
    }
    gettimeofday(&end, NULL);

    long total_usec = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    double per_op_ns = (total_usec * 1000.0) / iterations; // convert usec to nanoseconds, then per-op
    printf("10M tiny ops took %ld usec total, %.2f nanoseconds per op\n", total_usec, per_op_ns);
    // This "run N times and divide" trick is how you measure things faster than gettimeofday's resolution

    return 0;
}

// Compile and run:
//   gcc -o gettimeofday_test 10_gettimeofday_Test.c
//   ./gettimeofday_test
//
// Summary:
//   gettimeofday gives you microsecond resolution (10^-6 seconds).
//   For nanosecond resolution (10^-9), you'd use clock_gettime(CLOCK_MONOTONIC, ...) instead.
//   For measuring short things, run them millions of times and divide — don't try to measure one call.
