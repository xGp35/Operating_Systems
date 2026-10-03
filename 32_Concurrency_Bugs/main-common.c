// ============================================================================
// FILE MAP — how the pieces fit together:
//
//   main-header.h      → global config variables (loops, num_threads, flags)
//   vector-header.h    → defines vector_t struct (array of 100 ints + its own mutex lock)
//   common_threads.h   → wrapper macros around pthread functions (Pthread_create, etc.)
//   common.h           → utility macros (Time_GetSeconds, Malloc)
//   vector-deadlock.c  → the vector_add() function (the part that locks & adds)
//                         ↑ this file #include's main-common.c at the bottom,
//                         so main-common.c is compiled AS PART OF vector-deadlock.c
//
// In Python terms, imagine vector-deadlock.c doing:
//   from main_common import *
// That's what `#include "main-common.c"` does — it pastes this entire file in.
// ============================================================================

// Global array of vectors — shared across all threads.
// Python analogy: vectors = [Vector(i) for i in range(200)]
// We allocate 2 * MAX_THREADS = 200 vectors because with -p (parallelism),
// each thread gets its own PAIR of vectors (thread i uses v[2i] and v[2i+1]).
vector_t v[2 * MAX_THREADS];

// A separate lock just for printing. Without this, print output from different
// threads would interleave mid-line and become unreadable.
// Python analogy: print_lock = threading.Lock()
pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;

void usage(char *prog) {
    fprintf(stderr, "usage: %s [-d (turn on deadlocking behavior)] [-l loops] [-n num_threads] [-t (do timing)] [-v (for verbose)]\n", prog);
    exit(1);
}

// Initialize a vector: fill all 100 slots with `value` and set up its mutex.
// Python analogy:
//   def vector_init(v, value):
//       v.values = [value] * 100
//       v.lock = threading.Lock()
// Called only from main thread before any workers start, so no lock needed here.
void vector_init(vector_t *v, int value) {
    int i;
    for (i = 0; i < VECTOR_SIZE; i++) {
		v->values[i] = value; // All elements of the actual array within 'v', called 'values' are set to one particular integer, which came as input to this fn.
    }
    Pthread_mutex_init(&v->lock, NULL); 
}

// Print all 100 elements of a vector (for debugging). Not thread-safe.
void vector_print(vector_t *v, char *str) {
    int i;
    for (i = 0; i < VECTOR_SIZE; i++) {
		printf("%s[%d] %d\n", str, i, v->values[i]);
    }
}

// Debug logging: shows when a thread enters/exits vector_add.
// call_return=0 means "entering" (->), call_return=1 means "returning" (<-).
// Indentation increases with thread_id so you can visually see which thread is which.
// Only prints if -v (verbose) flag was passed.
//
// Example output with 2 threads and -v:
//   ->add(0, 1)          ← thread 0 entering vector_add(v[0], v[1])
//                 ->add(1, 0)   ← thread 1 entering vector_add(v[1], v[0])
void print_info(int call_return, int thread_id, int v0, int v1) {
    if (verbose == 0)
	return;
    Pthread_mutex_lock(&print_lock);
    int j;
    for (j = 0; j < thread_id; j++) printf("              ");
    if (call_return) 
	printf("<-add(%d, %d)\n", v0, v1);
    else
	printf("->add(%d, %d)\n", v0, v1);
    Pthread_mutex_unlock(&print_lock);
}

// This is NOT a thread itself — it's a bag of arguments we pass TO a thread.
// Think of it as the work order slip you hand to a worker.
//
// Python analogy:
//   @dataclass
//   class ThreadArgs:
//       tid: int               # thread's ID (0, 1, 2, ...)
//       vector_add_order: int  # 0 = normal order, 1 = reversed (causes deadlock)
//       vector_0: int          # index of the first vector in global array v[]
//       vector_1: int          # index of the second vector in global array v[]
typedef struct __thread_arg_t {
    int tid;
    int vector_add_order;
    int vector_0;
    int vector_1;
} thread_arg_t;

// The function each thread runs. This is what you'd pass to threading.Thread(target=...).
//
// Python analogy:
//   def worker(args: ThreadArgs):
//       for _ in range(loops):
//           if args.vector_add_order == 0:
//               vector_add(v[args.vector_0], v[args.vector_1])  # normal order
//           else:
//               vector_add(v[args.vector_1], v[args.vector_0])  # REVERSED — deadlock risk!
//
// `void *arg` is C's way of accepting any type — like Python's `def f(arg: Any)`.
// We immediately cast it back to the type we know it is (thread_arg_t *).
void *worker(void *arg) {
    // Cast the generic pointer back to our actual type.
    // Python equivalent: args = arg  (Python doesn't need casts)
    thread_arg_t *args = (thread_arg_t *) arg;
    int i, v0, v1;
    for (i = 0; i < loops; i++) {
	// THIS is where deadlock gets set up.
	// vector_add_order == 0: dst=vector_0, src=vector_1 (normal)
	// vector_add_order == 1: dst=vector_1, src=vector_0 (REVERSED)
	// When two threads use opposite orders, they lock in opposite order → deadlock.
	if (args->vector_add_order == 0) { 
	    v0 = args->vector_0;
	    v1 = args->vector_1;
	} else {
	    v0 = args->vector_1;
	    v1 = args->vector_0;
	}

	print_info(0, args->tid, v0, v1);

	// THE key call. vector_add() is defined in the including file
	// (e.g. vector-deadlock.c, vector-global-order.c, etc.)
	// Each file implements a DIFFERENT locking strategy for this operation.
	// v[v0] += v[v1], element-wise.
	vector_add(&v[v0], &v[v1]);

	print_info(1, args->tid, v0, v1);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    // ---- Parse command-line flags ----
    // Python analogy: parser = argparse.ArgumentParser()
    // This is C's manual version of argparse. getopt pulls one flag at a time.
    // "l:n:vtdp" means: -l and -n take a value (the colon), -v/-t/-d/-p are boolean.
    opterr = 0;
    int c;
    while ((c = getopt (argc, argv, "l:n:vtdp")) != -1) {
		switch (c) {
			case 'l':
				loops = atoi(optarg);       // how many times each thread repeats vector_add
				break;
			case 'n':
				num_threads = atoi(optarg);  // how many worker threads to spawn
				break;
			case 'v':
				verbose = 1;                 // print enter/exit traces
				break;
			case 't':
				do_timing = 1;               // print elapsed time at the end
				break;
			case 'd':
				cause_deadlock = 1;          // make odd threads reverse lock order
				break;
			case 'p':
				enable_parallelism = 1;      // give each thread its OWN pair of vectors
				break;
			default:
				usage(argv[0]);
		}
    }

    assert(num_threads < MAX_THREADS);

    // ---- Prepare thread arguments ----

    // pid[] stores thread handles. Like a list of threading.Thread objects.
    // Python analogy: threads = []
    pthread_t pid[MAX_THREADS];

    // args[] is the array of work orders — one per thread.
    // Python analogy: thread_args = [ThreadArgs(...) for i in range(num_threads)]
    thread_arg_t args[MAX_THREADS];
    int i;

    // Build each thread's work order: which vectors should it add?
    for (i = 0; i < num_threads; i++) {
		args[i].tid = i;

		if (enable_parallelism == 0) {
			// Without -p: ALL threads work on the SAME pair (v[0] and v[1]).
			// This means threads contend for the same locks — max lock contention.
			args[i].vector_0 = 0;
			args[i].vector_1 = 1;
		} else {
			// With -p: each thread gets its OWN pair (thread 0 → v[0],v[1];
			// thread 1 → v[2],v[3]; etc). No shared data → no contention → no deadlock.
			args[i].vector_0 = i * 2;
			args[i].vector_1 = i * 2 + 1;
		}

		// With -d: odd-numbered threads (1, 3, 5, ...) reverse the argument order.
		// Even threads call vector_add(v[0], v[1]) → locks v[0] then v[1].
		// Odd threads call  vector_add(v[1], v[0]) → locks v[1] then v[0].
		// Opposite lock orders on the same locks → deadlock.
		if (cause_deadlock && i % 2 == 1)
			args[i].vector_add_order = 1;
		else
			args[i].vector_add_order = 0;
    }

    // ---- Initialize all vectors ----
    // v[0] gets all 0s, v[1] gets all 1s, v[2] gets all 2s, etc.
    for (i = 0; i < 2 * MAX_THREADS; i++) 
	vector_init(&v[i], i);

    double t1 = Time_GetSeconds();  // start timer (defined in common.h)

    // ---- Launch threads ----
    // Python analogy:
    //   for i in range(num_threads):
    //       t = threading.Thread(target=worker, args=(thread_args[i],))
    //       t.start()
    //       threads.append(t)
    for (i = 0; i < num_threads; i++) 
	Pthread_create(&pid[i], NULL, worker, (void *) &args[i]);

    // ---- Wait for all threads to finish ----
    // Python analogy:
    //   for t in threads:
    //       t.join()
    // If deadlock occurred, the program hangs here forever — join never returns
    // because the threads are stuck waiting for each other's locks.
    for (i = 0; i < num_threads; i++) 
	Pthread_join(pid[i], NULL); 

    double t2 = Time_GetSeconds();  // stop timer

    // fini() is defined in the including file (e.g. vector-deadlock.c).
    // It's a cleanup hook — most implementations leave it empty.
    fini();
    if (do_timing) {
	printf("Time: %.2f seconds\n", t2 - t1);
    }

    //vector_print(&v[0], "v1");
    //vector_print(&v[1], "v2");
    return 0;
}
