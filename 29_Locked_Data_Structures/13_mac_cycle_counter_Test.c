#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <mach/mach_time.h>  // macOS-specific: mach_absolute_time

// On ARM (Apple Silicon), there's no RDTSC. The equivalent is:
//
// 1. CNTVCT_EL0 — a raw ARM instruction that reads the virtual timer counter.
//    Runs at 24 MHz on Apple Silicon (M1/M2/M3/M4), so each tick = ~41.67 ns.
//    That's much coarser than x86 RDTSC which ticks every ~0.3 ns on a 3GHz CPU.
//
// 2. mach_absolute_time() — Apple's official API that reads the same counter.
//    Returns ticks in an abstract unit. You convert to nanoseconds using mach_timebase_info.
//    This is the "right" way to do it on macOS. Low overhead (~4 ns).
//
// We'll show both approaches.

// --- Method 1: Raw ARM instruction (like RDTSC is raw x86) ---
// CNTVCT_EL0 = Counter-timer Virtual Count register
// ISB = Instruction Synchronization Barrier (like RDTSCP's serializing behavior —
//       forces all prior instructions to complete before reading the counter)
static inline uint64_t read_cntvct(void) {
    uint64_t val;
    __asm__ __volatile__("isb; mrs %0, cntvct_el0" : "=r"(val));
    return val;
}

// Read the timer frequency (CNTFRQ_EL0). On Apple Silicon this returns 24000000 (24 MHz).
static inline uint64_t read_cntfrq(void) {
    uint64_t val;
    __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(val));
    return val;
}

int main() {
    uint64_t start, end;
    uint64_t elapsed_ticks, overhead_ticks, total_ticks;
    double elapsed_ns, overhead_ns, total_ns, per_op_ns;
    int i;
    volatile int sum = 0;

    // Print the timer frequency so we know what we're working with
    uint64_t freq = read_cntfrq();
    printf("ARM timer frequency: %llu Hz (%.0f MHz)\n", (unsigned long long)freq, freq / 1000000.0);
    // On M4 with macOS 15+: 1000000000 Hz (1 GHz) — Apple now scales the counter to 1 GHz
    // On older macOS: 24000000 Hz (24 MHz) — each tick = ~41.67 nanoseconds
    printf("\n");

    // =============================================
    // Part A: Using raw CNTVCT_EL0 (ARM's "RDTSC")
    // =============================================
    printf("=== CNTVCT_EL0 (raw ARM counter) ===\n");

    // Quick sanity check — CNTVCT_EL0 may not work in VMs or emulated environments
    uint64_t sanity1 = read_cntvct();
    uint64_t sanity2 = read_cntvct();
    if (sanity2 <= sanity1) {
        printf("CNTVCT_EL0 not working on this system (VM/emulation issue). Skipping.\n\n");
    } else {

    // --- Example 1: Measure a simple loop in ticks ---
    start = read_cntvct();

    for (i = 0; i < 1000000; i++) {
        sum += i;
    }

    end = read_cntvct();

    elapsed_ticks = end - start;
    elapsed_ns = (double)elapsed_ticks / freq * 1000000000.0; // convert ticks to nanoseconds
    printf("Loop of 1,000,000 additions: %llu ticks (%.3f ms)\n",
           (unsigned long long)elapsed_ticks, elapsed_ns / 1000000.0);

    // --- Example 2: Overhead of CNTVCT_EL0 itself ---
    start = read_cntvct();
    end = read_cntvct();

    overhead_ticks = end - start;
    overhead_ns = (double)overhead_ticks / freq * 1000000000.0;
    printf("Two back-to-back reads: %llu ticks (%.1f ns)\n",
           (unsigned long long)overhead_ticks, overhead_ns);
    // At 24 MHz, each tick is ~41.67 ns, so this might show 0 or 1 tick.
    // At 1 GHz (newer macOS), each tick = 1 ns — much finer.
    // Unlike RDTSC which ticks at CPU frequency (~3 GHz), this counter is still coarser.

    // --- Example 3: Per-operation measurement ---
    int iterations_a = 10000000;
    start = read_cntvct();
    for (i = 0; i < iterations_a; i++) {
        sum += 1;
    }
    end = read_cntvct();

    total_ticks = end - start;
    total_ns = (double)total_ticks / freq * 1000000000.0;
    per_op_ns = total_ns / iterations_a;
    printf("10M tiny ops: %llu ticks total, %.2f ns per op\n",
           (unsigned long long)total_ticks, per_op_ns);

    } // end sanity check
    printf("\n");

    // =============================================
    // Part B: Using mach_absolute_time (Apple's API)
    // =============================================
    printf("=== mach_absolute_time (macOS API) ===\n");

    // mach_absolute_time returns ticks in an abstract unit.
    // mach_timebase_info tells you how to convert: nanoseconds = ticks * numer / denom
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    printf("Timebase: %u / %u (multiply ticks by this to get nanoseconds)\n",
           timebase.numer, timebase.denom);

    // --- Example 1: Measure a simple loop ---
    int iterations = 10000000;
    start = mach_absolute_time();
    sum = 0;
    for (i = 0; i < 1000000; i++) {
        sum += i;
    }
    end = mach_absolute_time();

    elapsed_ticks = end - start;
    elapsed_ns = elapsed_ticks * timebase.numer / timebase.denom;
    printf("Loop of 1,000,000 additions: %llu ticks (%.3f ms)\n",
           (unsigned long long)elapsed_ticks, elapsed_ns / 1000000.0);

    // --- Example 2: Overhead ---
    start = mach_absolute_time();
    end = mach_absolute_time();

    overhead_ticks = end - start;
    overhead_ns = overhead_ticks * timebase.numer / timebase.denom;
    printf("Two back-to-back reads: %llu ticks (%.1f ns)\n",
           (unsigned long long)overhead_ticks, overhead_ns);

    // --- Example 3: Per-operation ---
    start = mach_absolute_time();
    for (i = 0; i < iterations; i++) {
        sum += 1;
    }
    end = mach_absolute_time();

    total_ticks = end - start;
    total_ns = (double)(total_ticks * timebase.numer) / timebase.denom;
    per_op_ns = total_ns / iterations;
    printf("10M tiny ops: %llu ticks total, %.2f ns per op\n", (unsigned long long)total_ticks, per_op_ns);

    return 0;
}

// Compile and run (macOS Apple Silicon only):
//   gcc -o mac_cycle_test 13_mac_cycle_counter_Test.c
//   ./mac_cycle_test
//
// This file is the Mac counterpart of 12_rdtsc_Test.c (which is x86-only).
//
// Summary of all timing methods across platforms:
//   x86 Linux:   gettimeofday → clock_gettime → RDTSC           (coarse → fine)
//   ARM macOS:   gettimeofday → clock_gettime → mach_absolute_time / CNTVCT_EL0
//
// Key difference: RDTSC ticks at CPU frequency (~3 GHz = ~0.3 ns per tick)
//                 CNTVCT_EL0 ticks at 24 MHz (~41.67 ns per tick)
// So ARM's hardware counter is ~125x coarser than x86's.
// For most benchmarking, mach_absolute_time or clock_gettime is good enough.
