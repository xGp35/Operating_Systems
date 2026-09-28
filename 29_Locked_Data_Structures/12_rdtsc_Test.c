#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>  // for uint64_t, uint32_t

// RDTSC = Read Time Stamp Counter
// This is not a library call — it's a raw CPU instruction.
// It reads a 64-bit counter built into the CPU that increments every clock cycle.
// So it measures in cycles, not seconds or nanoseconds.
//
// To convert cycles to time, you need to know the CPU frequency:
//   time_in_seconds = cycles / cpu_frequency_hz
//
// Why use RDTSC over gettimeofday/clock_gettime?
//   gettimeofday  → syscall overhead, microsecond resolution
//   clock_gettime → syscall overhead (sometimes vDSO), nanosecond resolution
//   RDTSC         → no syscall, ~1 cycle overhead, sub-nanosecond resolution
// It's the fastest possible way to get a timestamp on x86/x86_64.
//
// Gotcha: on modern CPUs, RDTSC counts at a fixed "reference" frequency,
// not the actual clock speed (which changes with turbo boost / power saving).
// So "cycles" from RDTSC aren't always real CPU cycles — they're reference cycles.

// Inline assembly to read the timestamp counter.
// RDTSC puts the lower 32 bits in EAX and upper 32 bits in EDX.
// We combine them into a single 64-bit value.
static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

// RDTSCP is a serializing version of RDTSC.
// Regular RDTSC can be reordered by the CPU (out-of-order execution might
// read the counter before/after the instructions you're trying to measure).
// RDTSCP waits until all prior instructions have completed before reading.
// Use RDTSCP for the "end" measurement to get accurate results.
static inline uint64_t rdtscp(void) {
    uint32_t lo, hi, aux;
    __asm__ __volatile__ ("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux));
    return ((uint64_t)hi << 32) | lo;
}

int main() {
    uint64_t start, end;

    // --- Example 1: Measure a simple loop in CPU cycles ---
    start = rdtsc();

    int i;
    volatile int sum = 0;
    for (i = 0; i < 1000000; i++) {
        sum += i;
    }

    end = rdtscp(); // rdtscp at the end to prevent reordering

    uint64_t elapsed_cycles = end - start;
    printf("Loop of 1,000,000 additions took %llu cycles\n", (unsigned long long)elapsed_cycles);

    // --- Example 2: Overhead of RDTSC itself ---
    // Call it twice back-to-back. This shows how many cycles the instruction itself costs.
    start = rdtsc();
    end = rdtscp();

    uint64_t overhead = end - start;
    printf("Two back-to-back rdtsc/rdtscp calls: %llu cycles apart\n", (unsigned long long)overhead);
    // Typically ~20-40 cycles. Compare this with clock_gettime which was ~30 nanoseconds.
    // On a 3GHz CPU, 30ns = ~90 cycles. So RDTSC is faster than clock_gettime.

    // --- Example 3: Measure something tiny — cycles per operation ---
    int iterations = 10000000; // 10 million
    start = rdtsc();
    for (i = 0; i < iterations; i++) {
        sum += 1;
    }
    end = rdtscp();

    uint64_t total_cycles = end - start;
    double per_op_cycles = (double)total_cycles / iterations;
    printf("10M tiny ops took %llu cycles total, %.2f cycles per op\n",
           (unsigned long long)total_cycles, per_op_cycles);

    return 0;
}

// Compile and run (x86/x86_64 only — won't work on ARM/Apple Silicon!):
//   gcc -o rdtsc_test 12_rdtsc_Test.c
//   ./rdtsc_test
//
// NOTE: RDTSC is an x86 instruction. It does NOT exist on ARM (Apple M1/M2/M3/M4).
// This will only compile and run on your EC2 Linux instance (x86_64), not on your Mac.
//
// Comparison of all three timing methods:
//   gettimeofday  → microseconds  → syscall         → ~1 usec overhead
//   clock_gettime → nanoseconds   → syscall/vDSO    → ~30 ns overhead
//   RDTSC         → CPU cycles    → raw instruction  → ~20-40 cycles overhead (~7-13 ns on 3GHz)
