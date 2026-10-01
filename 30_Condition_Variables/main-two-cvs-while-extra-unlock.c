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

pthread_cond_t empty  = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill   = PTHREAD_COND_INITIALIZER;
pthread_mutex_t m     = PTHREAD_MUTEX_INITIALIZER;

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
	    Cond_wait(&empty, &m);  p3;
	}
	Mutex_unlock(&m);
	do_fill(base + i);          p4;
	Mutex_lock(&m);
	Cond_signal(&fill);         p5;
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
	    Cond_wait(&fill, &m);  c3;
        }
	Mutex_unlock(&m);
	tmp = do_get();            c4;
	Mutex_lock(&m);
	Cond_signal(&empty);       c5;
	Mutex_unlock(&m);          c6;
	consumed_count++;
    }

    // return consumer_count-1 because END_OF_STREAM does not count
    return (void *) (long long) (consumed_count - 1);
}

// must set these appropriately to use "main-common.c"
pthread_cond_t *fill_cv = &fill;
pthread_cond_t *empty_cv = &empty;

// all codes use this common base to start producers/consumers
// and all the other related stuff
#include "main-common.c"

/*
The key difference from `main-two-cvs-while.c` is that the mutex is **released before** `do_fill`/`do_get`:

```c
// Producer                          // Consumer
Mutex_lock(&m);                      Mutex_lock(&m);
while (num_full == max) {            while (num_full == 0) {
    Cond_wait(&empty, &m);               Cond_wait(&fill, &m);
}                                    }
Mutex_unlock(&m);     // ← UNLOCK   Mutex_unlock(&m);     // ← UNLOCK
do_fill(base + i);    // NO LOCK!   tmp = do_get();       // NO LOCK!
Mutex_lock(&m);                      Mutex_lock(&m);
Cond_signal(&fill);                  Cond_signal(&empty);
Mutex_unlock(&m);                    Mutex_unlock(&m);
```

`do_fill` and `do_get` modify shared state (`buffer`, `fill_ptr`, `use_ptr`, `num_full`) **without any lock protection**.

## The race condition

Consider 1 producer, 2 consumers, buffer size 1:

1. Producer fills item 0 (`num_full=1`), signals fill
2. **CT1**: wakes from `Cond_wait`, acquires mutex, checks `while(num_full == 0)` → FALSE. **Unlocks**. About to call `do_get`...
3. **CT2** was blocked at `Mutex_lock`. CT1 just unlocked → CT2 gets the mutex. Checks `while(num_full == 0)` → **FALSE** (num_full is still 1 because CT1 hasn't run `do_get` yet -- it's outside the lock!). CT2 also **unlocks**.
4. **Both CT1 and CT2** call `do_get()` concurrently on the same slot.

The `ensure(tmp != EMPTY)` assertion catches this: the second consumer reads a slot already set to `EMPTY` by the first.

## Bad things that can happen

- **Double-read**: two consumers read the same buffer slot, one gets the real value, the other gets `EMPTY` → crash via `ensure`
- **Corrupted `use_ptr`/`fill_ptr`**: both threads increment the same pointer simultaneously → values get skipped or read twice
- **Corrupted `num_full`**: `num_full--` is not atomic. Two concurrent decrements can result in only one actual decrement → count drifts from reality
- **Lost values**: with 2 producers, both write to the same `fill_ptr` slot → one value is silently overwritten

## Triggering it reliably

The race window is between `Mutex_unlock` and `do_fill`/`do_get` -- and there's no pause point there (c4/p4 are **after** the operation). So we can't directly force a thread to stall in that window with sleep strings.

But we can maximize the probability by running **more threads** to increase contention. Try:

```bash
# 2 consumers competing for the same slot
./main-two-cvs-while-extra-unlock.out -l 100 -m 1 -p 1 -c 2 -v

# 2 producers competing to fill the same slot
./main-two-cvs-while-extra-unlock.out -l 100 -m 1 -p 2 -c 1 -v

# Maximum chaos
./main-two-cvs-while-extra-unlock.out -l 100 -m 1 -p 2 -c 2 -v
```

With buffer size 1, the race window is tightest (two threads going for the exact same slot). Running with `-l 100` gives 100 opportunities for the race to manifest. You should see either the `ensure` assertion firing or corrupted output.

The core lesson: the `while` loop correctly re-checks the condition, but that's pointless if you release the lock before acting on it. Between the unlock and the actual operation, another thread can sneak in and invalidate the condition you just checked. **The check and the action must be atomic** (inside the same critical section).
*/

/*
It's strange, like when I run without the minus verbose flag, the minus v flag, I am able to get that try to get an empty buffer error, but if I run it with the verbose flag, I'm always never able to get it. So this is the command I use to get an empty buffer.

~/Jupyter/Projects/Operating_Systems/30_Condition_Variables main
❯ ./main-two-cvs-while-extra-unlock.out -l 50 -m 10 -p 1 -c 10 -t 
error: tried to get an empty buffer
error: tried to get an empty buffer

~/Jupyter/Projects/Operating_Systems/30_Condition_Variables main
❯ ./main-two-cvs-while-extra-unlock.out -l 50 -m 10 -p 1 -c 10 -t
error: tried to get an empty buffer

This makes me think the verbose flag is printing things to the STD out. Is that causing like fewer issues than previous cases where I was also unable to create a red condition or a deadlock? That might be possible if I just remove the verbose flag, the scheduler can act more freely at that time, I guess. Printing probably takes time.

You've discovered a classic phenomenon in concurrent programming called a **Heisenbug** -- a bug that disappears when you try to observe it.

Your intuition is exactly right. Look at what `-v` does in `do_pause`:

`@/Users/gprasanna/Jupyter/Projects/Operating_Systems/30_Condition_Variables/main-header.h:86-98`

Every pause point acquires `print_lock`, does multiple `printf` calls, then releases `print_lock`. This has two effects:

1. **I/O is slow.** `printf` to a terminal is orders of magnitude slower than a memory operation. Each trace line takes microseconds to milliseconds. The race window between `Mutex_unlock` and `do_get` is nanoseconds. The printing drowns out the race.

2. **Extra serialization.** All threads contend for `print_lock`. This effectively forces threads to take turns printing, which inadvertently serializes their execution. The threads spend most of their time waiting for `print_lock` or doing I/O, not racing on the buffer.

Without `-v`, the threads run at full speed -- `do_pause` returns immediately (the `if (do_trace == 0) return;` check at the top). The race window is fully exposed, and 10 consumers slamming into the same buffer slot with no lock triggers the error almost instantly.

This is also likely relevant to your earlier experiments. The one-CV deadlock and other race conditions may be significantly easier to trigger without `-v`, because the printing changes scheduling behavior so much that it masks the bugs.
*/
