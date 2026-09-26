Both — and the conversation you've had so far has actually already surfaced the specific mechanism behind each half. Let me tie it together explicitly since that's what the book is really asking you to synthesize.

**Performance**

In `main-signal.c`, the parent spins in `while (done == 0);`, and as you worked out earlier, this consumes 100% of a CPU core (or, on a single-core machine, actively steals scheduling time away from the worker thread it's waiting on) for the entire duration the worker takes to finish. If the worker takes 10 seconds, you burn 10 seconds of CPU doing absolutely nothing useful.

In the condition-variable version, `pthread_cond_wait` puts the calling thread to **sleep** — it's removed from the run queue entirely and consumes zero CPU while blocked. The OS only reschedules it once `pthread_cond_signal` actually wakes it. So the efficiency gain isn't marginal — it's the difference between "burn a full CPU core for the whole wait" and "consume zero CPU while waiting, get scheduled only when there's real work to do."

**Correctness**

This is the part your Helgrind experiment actually demonstrated empirically. `main-signal.c` has:

- **A real, formally-defined data race** on `done` — Helgrind flagged it because there's no synchronization establishing a happens-before edge between the worker's write and the parent's read. That's undefined behavior in the C standard, not just messy style.
- **A compiler-legality problem**: without `volatile`, `while (done == 0);` can legally be hoisted by the compiler into a load-once-then-infinite-loop, since nothing inside the loop body writes `done` as far as the compiler can see.

The condition-variable version fixes both, and for a reason you already reasoned through carefully: `pthread_mutex_lock`/`unlock` around every access to `done` act as a compiler barrier (forcing re-reads from memory every iteration) *and* establish a proper acquire/release synchronizes-with relationship recognized by the memory model — so there's no UB, and Helgrind would report nothing on this version.

**Why "both" is the honest answer, not a hedge**

It's worth resisting the temptation to pick just one, because the two properties are somewhat independent, and the book is testing whether you see that:

- You could imagine a "correct but still slow" fix — e.g., wrap `done` in a mutex but keep spinning: `lock(); while(done==0){unlock(); lock();} unlock();`. That would eliminate the data race (correctness fixed) but still burn CPU busy-polling (performance not fixed).
- Conversely, you could imagine something "fast but still broken" — e.g., some ad hoc signal-based scheme without proper locking that happens to sleep the thread but still races on shared data.

The condition-variable pattern is preferred specifically because `pthread_cond_wait` is designed to solve *both* problems with a single primitive: it sleeps the thread (eliminating the busy-wait, fixing performance) while atomically coupled to a mutex acquire/release (eliminating the race, fixing correctness). That dual guarantee — that the same call that removes you from the CPU also gives you the synchronization contract that makes the shared-variable access legal — is exactly why OSTEP treats this as the canonical answer, rather than treating locking and sleeping as two separate problems you'd solve with two separate tools.