#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>

#include "common.h"
#include "common_threads.h"

#include "pc-header.h"

pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

#include "main-header.h"

void do_fill(int value) {
    // ensure empty before usage
    ensure(buffer[fill_ptr] == EMPTY, "error: tried to fill a non-empty buffer");
    buffer[fill_ptr] = value;
    fill_ptr = (fill_ptr + 1) % max;
    num_full++;
}

int do_get() {
    int tmp = buffer[use_ptr];
    ensure(tmp != EMPTY, "error: tried to get an empty buffer");
    buffer[use_ptr] = EMPTY; 
    use_ptr = (use_ptr + 1) % max;
    num_full--;
    return tmp;
}

void *producer(void *arg) {
    int id = (int)(long long) arg;
    // make sure each producer produces unique values
    int base = id * loops; 
    int i;
    for (i = 0; i < loops; i++) {   p0;
	Mutex_lock(&m);             p1;
	while (num_full == max) {   p2;
	    Cond_wait(&cv, &m);     p3;
	}
	do_fill(base + i);          p4;
	Cond_signal(&cv);           p5;
	Mutex_unlock(&m);           p6;
    }
    return NULL;
}
                                                                               
void *consumer(void *arg) {
    int id = (int)(long long) arg;
    int tmp = 0;
    int consumed_count = 0;
    while (tmp != END_OF_STREAM) { c0;
	Mutex_lock(&m);            c1;
	while (num_full == 0) {    c2;
	    Cond_wait(&cv, &m);    c3;
        }
	tmp = do_get();            c4;
	Cond_signal(&cv);          c5;
	Mutex_unlock(&m);          c6;
	consumed_count++;
    }

    // return consumer_count-1 because END_OF_STREAM does not count
    return (void *) (long long) (consumed_count - 1);
}

// must set these appropriately to use "main-common.c"
pthread_cond_t *fill_cv = &cv;
pthread_cond_t *empty_cv = &cv;

// all codes use this common base to start producers/consumers
// and all the other related stuff
#include "main-common.c"

/*
No worries, let me continue. Let me first re-read the one-cv-while code to reason precisely:

OK, let me think through this carefully. The key issue with `main-one-cv-while.c` is that **everyone signals the same CV**. The classic deadlock is:

## The deadlock scenario you want

1. CT1 and CT2 both in `Cond_wait` (buffer empty)
2. Producer fills, signals CV → wakes CT1
3. **Producer wins the mutex race**, re-acquires mutex before CT1, sees `num_full == max` → enters `Cond_wait`
4. CT1 gets mutex (producer's Cond_wait released it), consumes, signals CV
5. Signal wakes **CT2 instead of producer** (both on same CV)
6. CT2 checks `while(num_full == 0)` → TRUE → goes back to `Cond_wait`
7. CT1 loops, finds buffer empty → enters `Cond_wait`
8. All three threads in `Cond_wait`. **DEADLOCK.**

## Why your `-P 0,0,0,0,0,0,3` hurts rather than helps

The p6 sleep of 3 seconds happens **after** the producer unlocks. During those 3 seconds, the consumer easily consumes and the buffer becomes empty. When the producer wakes and loops back, it finds `num_full = 0`, fills immediately, and **never enters Cond_wait**. You've eliminated step 3 -- the producer never gets stuck.

The deadlock only happens late (by luck) when the scheduling stars eventually align despite the p6 sleep working against you.

## The core problem

Step 3 requires the **producer to win the mutex race** after p6 (unlock). But the woken consumer (CT1) is also trying to re-acquire the mutex from `Cond_wait`. On most systems, the signaled thread gets priority for the mutex. No sleep parameter can control this race because:

- Any producer sleep after unlock (p6) makes the producer **slower** → loses the race
- Any consumer sleep at c3 holds the **mutex** → producer can't enter Cond_wait during it
- Sleeps at c0/c6 are outside the critical window

## What would maximize your chances

Ironically, **no sleep at all** gives the best odds:

```
./main-one-cv-while.out -l 10 -m 1 -p 1 -c 2 -v -t
```

Run it many times. The producer sometimes wins the mutex race by sheer scheduling luck. On macOS with its bursty scheduler, the producer (currently running) is more likely to keep the CPU and loop back to `Mutex_lock` before the woken consumer gets scheduled. On Linux with EEVDF, the woken consumer almost always gets priority, making the deadlock much rarer.

The uncomfortable truth: this deadlock is **genuinely non-deterministic**. Sleep parameters can set up the initial conditions (both consumers in Cond_wait) but can't control the two critical coin flips: who wins the mutex race, and which thread gets woken by the signal.
*/