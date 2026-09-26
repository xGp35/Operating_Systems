#include <stdio.h>

#include "common_threads.h"

volatile int done = 0;

void* worker(void* arg) {
    printf("this should print first\n");
    done = 1;
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t p;
    Pthread_create(&p, NULL, worker, NULL);
    while (done == 0)
	;
    printf("this should print last\n");
    return 0;
}

//Every time the main thread gets CPU time, it performs essentially zero useful work.
//That's called busy waiting (or spinning).
//The important point is that the main thread doesn't sleep while waiting.

// this snippet is also unsafe because done isn't declared volatile — a compiler is technically permitted to cache done in a register in main since nothing in the loop body appears to modify it, producing an infinite loop even after the write happens.

// Your intuition is: "sure, it's technically unsynchronized, but the logical outcome here is deterministic — worker always sets done=1 after its print, and main only exits the loop after seeing done==1, so the ordering is always correct regardless of scheduling." That reasoning is sound if you assume sequential consistency — that all memory operations become visible to all threads in program order. But that assumption is exactly what's not guaranteed once you drop synchronization, and here's where "benign-looking" data races bite you in practice:

//The compiler is allowed to break this. Without volatile or an atomic type, the compiler sees a loop while (done == 0); where nothing in the loop body writes done. From its point of view (it doesn't reason about other threads unless you use synchronization primitives), it's free to load done into a register once and never recheck memory — turning your loop into if (done == 0) while(1);. This isn't hypothetical pessimism; it's permitted behavior, and people do get bitten by it.

// Even ignoring the compiler, the hardware/memory model doesn't promise timely visibility. On x86 you usually get away with it because of cache coherence, but the C/C++ language standard classifies an unsynchronized conflicting access as undefined behavior, not "well-defined but you should still add a lock for style points." Undefined behavior means the standard makes zero guarantees — it's not "probably fine," it's "outside the contract."