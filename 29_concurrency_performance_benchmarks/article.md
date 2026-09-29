# I Threw 1,024 Threads at a Single Lock. Here's What Actually Happened.

*A journey from a simple homework question into CPU architecture, NUMA topology, and the honest truth about concurrent data structures.*

---

I'm working through [OSTEP](https://pages.cs.wisc.edu/~remzi/OSTEP/) (Operating Systems: Three Easy Pieces), and Chapter 29 on locked data structures has this homework question:

> Build a simple concurrent counter and measure how long it takes to increment the counter many times as the number of threads increases. How many CPUs are available on the system you are using? Does this number impact your measurements at all?

Sounds straightforward. Build a counter with a lock, throw threads at it, measure time. I expected a clean, satisfying graph showing "more threads = slower." I got something much more interesting.

---

## The Counter

The concurrent counter is about as simple as concurrent code gets:

```c
void counter_increment(counter_t *c) {
    Pthread_mutex_lock(&c->mutex);
    c->value++;
    Pthread_mutex_unlock(&c->mutex);
}
```

Three lines. Lock, increment, unlock. Every thread that wants to increment the counter has to wait its turn. Only one thread can be inside the lock at any given moment.

The benchmark keeps the **total work constant** at 10 million increments. With 1 thread, it does all 10M. With 4 threads, each does 2.5M. With 1,024 threads, each does about 9,765. Same total work, just split differently.

The question is: does splitting the work across more threads make it faster (parallelism!) or slower (contention!)?

---

## The Setup

Two machines:
- **Mac M4 Pro** — 12 CPUs (8 performance + 4 efficiency cores), Apple Silicon ARM
- **AWS EC2** — 32 CPUs (16 physical cores with hyperthreading), AMD EPYC 7R13 (Zen 3)

Timer: `clock_gettime(CLOCK_MONOTONIC)` — nanosecond resolution, works on both platforms.

I compiled with `-O0` (no optimization) so the compiler wouldn't outsmart my benchmark by optimizing away the lock or the increment.

---

## First Results: Mac M4 Pro

![Mac M4 Pro benchmark results](results/article_mac_plot.png)

The shape tells the story:

| Threads | Time (ms) | What's happening |
|---------|-----------|-----------------|
| 1 | 44 | No contention. Lock + increment + unlock, 10 million times straight through. |
| 2 | 66 | Small increase. Two threads, but the critical section is so tiny they rarely collide. |
| 4 | 134 | **Big jump.** Now threads are constantly waiting for each other. |
| 8-512 | ~120-140 | **Plateau.** The lock is saturated — adding more threads doesn't make it worse. |
| 1024 | 160 | Slight increase from OS overhead of managing 1,024 thread stacks and scheduling. |

The main takeaway: **going from 1 to 4 threads makes the same work take 3x longer.** After that, it doesn't matter much whether you have 4 threads or 512 — the single lock is the bottleneck, and only one thread can pass through it at a time.

Think of it like a turnstile at a stadium. Once there's a constant queue, the throughput of the turnstile is the same whether 10 people or 500 people are in line.

---

## Linux Results: Where Things Get Noisy

I ran the same benchmark on the EC2 instance. Here's where my clean narrative started to fall apart.

![Mac vs Linux comparison](results/article_comparison_plot.png)

Two things jumped out:

**1. Linux was consistently slower than Mac, even with more CPUs.**

The M4 Pro finished 10M increments with 1 thread in 44ms. The AMD EPYC took 80ms. This isn't a "Mac is better" thing — the M4 Pro's P-cores run at 4.5 GHz, while the EPYC runs at 2.65 GHz. The per-core speed difference explains most of it.

**2. The Linux results were wildly inconsistent between runs.**

I ran the same benchmark 5 times. Here's what 2 threads looked like:

| Run | Time (ms) |
|-----|-----------|
| 1 | 237 |
| 2 | 234 |
| 3 | 193 |
| 4 | 160 |
| 5 | 214 |

From 160ms to 237ms — a **48% swing** — for the exact same code. On Mac, runs varied by maybe 5%. What was going on?

---

## Down the Rabbit Hole: Thread Placement

I suspected the OS scheduler was placing threads on different CPUs each run, and where threads land matters. To test this, I wrote a new benchmark that **pins threads to specific CPUs** using `pthread_setaffinity_np`.

The AMD EPYC 7R13 has a specific internal layout:

```
AMD EPYC 7R13 (as seen on this EC2 instance)
┌──────────────────────────────────┐
│  NUMA Node 0                     │
│  8 physical cores (each with     │
│  a hyperthread sibling)          │
│  Shared 32 MB L3 cache           │
├──────────────────────────────────┤
│  NUMA Node 1                     │
│  8 physical cores (each with     │
│  a hyperthread sibling)          │
│  Shared 32 MB L3 cache           │
└──────────────────────────────────┘
  Connected by Infinity Fabric
```

Cores within a NUMA node share a fast L3 cache. Cores across NUMA nodes communicate through a slower interconnect called "Infinity Fabric." The OS scheduler decides where to put your threads, and it makes different decisions each run.

Here's what the pinned experiments revealed:

![Thread placement experiments](results/article_numa_plot.png)

### 2 Threads: A 5.5x Difference Based on Placement

| Placement | Time (ms) |
|-----------|-----------|
| Same physical core (hyperthreads) | **944** |
| Different cores, same NUMA node | **170** |
| Different NUMA nodes | **236** |

The same code. The same 2 threads. The same 10M increments. But a **5.5x difference** depending on which CPUs the OS happens to pick.

**Why is same-core (hyperthreads) so terrible?** Two hyperthreads share a single physical core's execution pipeline. They take turns using it. For a tight lock/unlock loop, each thread only gets half the pipeline. It's like two people sharing one checkout lane — they have to alternate for every single item.

**Why is same-NUMA faster than cross-NUMA?** When one thread modifies the counter and releases the lock, the other thread needs to read the updated value. If both are in the same NUMA node, the data travels through the shared L3 cache (fast). If they're on different NUMA nodes, the data travels through the Infinity Fabric interconnect (slower).

This explained the Linux variability. Each run, the scheduler made different placement decisions, causing 160ms on a good run and 237ms on a bad one.

---

## The Surprise: 16 Threads Was Faster Than 8

While running the pinned experiments, I stumbled on something I didn't expect:

| Configuration | Time (ms) |
|---------------|-----------|
| 8 threads on 1 NUMA node (8 physical cores) | **220** |
| 8 threads split 4+4 across NUMA | **240** |
| 16 threads, 8+8 across NUMA | **160** |

16 threads was **28% faster** than 8 threads, doing the exact same total work with the same lock.

I ran it 4 separate times. Every single time, 16 threads on 16 physical cores across both NUMA nodes was the fastest configuration. The absolute times varied with system load, but the relative ordering never changed.

**This shouldn't happen.** More threads fighting over one lock should mean more contention, more waiting, slower overall. The lock can only let one thread through at a time — having more threads in line shouldn't make the line move faster.

My best explanation: when 8 threads are crammed on 8 cores within one NUMA node, all of them are actively spinning on the lock (doing rapid atomic compare-and-swap operations). This creates a firestorm of cache coherence traffic — 7 cores all demanding exclusive access to the same cache line, overwhelming that NUMA node's L3 cache coherence engine. When threads spread across both NUMA nodes, most contending threads go to sleep rather than spin, reducing the hardware-level traffic. Two L3 caches share the coherence load instead of one being hammered.

I'm not 100% certain this is the full explanation. It may involve subtleties of the Linux futex implementation or the AMD Infinity Fabric's behavior under atomic contention. But the data is consistent and reproducible.

---

## What I Actually Learned

Going in, I expected this to be a 30-minute exercise. Write the counter, run the benchmark, make a graph, answer the homework question. Instead, I spent hours going down rabbit holes about timer resolution, CPU frequency scaling, NUMA topology, and cache coherence protocols.

Here's the honest summary.

### What IS clear:

1. **A single-lock concurrent counter gets worse with threads.** Going from 1 to a few threads makes the same work take 3-4x longer. This is the answer to the OSTEP question.

2. **It plateaus quickly.** Once the lock is saturated (a few threads), adding hundreds more threads barely changes the total time. The lock is the bottleneck, and its throughput is fixed.

3. **Thread placement matters enormously.** The same 2-thread benchmark ranged from 170ms to 944ms depending on which CPUs the threads landed on. This is invisible to the programmer — the OS scheduler makes this choice, and it varies between runs.

### What IS hazy:

The exact transition from 1 to 2 to 3 to 4 threads. Sometimes 2 threads was close to 1 thread (on Mac). Sometimes it jumped immediately (on Linux). This depends on how the OS places threads, what else is running, the specific mutex implementation, and architecture-specific cache coherence behavior.

And that, I think, is actually the most important lesson. **Concurrent performance is not deterministic in the way sequential code is.** The same code, same inputs, same hardware can give you meaningfully different numbers from run to run. If you're benchmarking concurrent code, run it many times and look at distributions, not single data points.

### The real point of the exercise:

The OSTEP chapter introduces the concurrent counter not because it's useful — it's terrible — but because it motivates the *approximate counter* (also called the sloppy counter), where each CPU has its own local counter and they periodically sync. That design eliminates the single-lock bottleneck entirely. Understanding *why* the simple counter is bad makes the clever solution click.

---

## Setup & Reproducibility

All code is in a single self-contained project:
- `counter_benchmark.c` — the benchmark (C, pthreads, clock_gettime)
- `plot_results.py` — plotting script (Python + matplotlib, uses `uv` inline dependencies)
- `run_benchmarks.sh` — compile, run, and plot in one command

To reproduce:
```bash
./run_benchmarks.sh
```

The benchmark auto-detects your CPU count and inserts it into the thread count list, so you can see where your specific hardware's transition point lies.

---

*This experiment was part of working through OSTEP Chapter 29: Lock-based Concurrent Data Structures. The counter benchmark project and all results are available on [GitHub](https://github.com/).*
