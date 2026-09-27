int FetchAndAdd(int *ptr) {
    int old = *ptr;
    *ptr = old + 1;
    return old;
}

typedef struct __lock_t { 
    int ticket;
    int turn;
} lock_t;

void lock_init(lock_t *mutex) {
    mutex->ticket = 0;
    mutex->turn= 0;
}

void lock(lock_t *mutex) {
    int myturn = FetchAndAdd(&mutex->ticket); // we are passing address of mutex->ticket to FetchAndAdd
    while (mutex->turn != myturn) 
        ; //spin-wait (do nothing)
}

void unlock(lock_t *mutex) {
    mutex->turn = mutex->turn + 1;
}


// One thread calls lock(&mutex), assume mutex was unlocked, whatever that means in this FetchAndAdd.
// Now FetchAndAdd(&mutex->ticket) runs.
// ticket was 0. so, ticket increased to 1. And 0 is returned.
// myturn = 0
// mutex->turn = 0, myturn =0, They are equal. So, Thread 1 has "got the lock". and ticket = 1

// Now another thread tries lock(&mutex), before thread 1 has released its lock. 
// Now FetchAndAdd(&mutex->ticket) runs
// ticket was 1, so ticket is increased to 2. 1 is returned. So, myturn = 1
// mutex->turn = 0, myturn = 1, unequal, Thread 2 stuck in spin-wait.

//Thread 3 tries the same. mutex->ticket = 3, 2 is returned. myturn = 2
// mutex->turn = 0.  mutex->turn != myturn, so thread 3 also goes to spin loop.

// Say Thread 1 unlocks. so mutex->turn = 1
// Now for Thread 2, mutex->turn becomes equal to myturn(1). So it can continue to run the critical section.
// For Thread 3, it will only be available after Thread 2 has released.

// Okay so this is a turn based algo where a particular thread will get to run if it myturn is 
// equal to mutex->turn.
// the mutex->turn keeps increasing after each thread releases the lock. So, each thread will get its time on the critical section
// First Thread to ask for lock say Thread 1, gets myturn = 0. when it unlocks, it sets mutex->turn to 1
// Thread 2 gets myturn = 1, when it unlocks, it sets mutex->turn to 2
// Thread 3 gets myturn = 2, when it unlocks, it sets mutex->turn to 3

// next time if Thread 1 wants to run again, it will get a different value of myturn. say after Thread 2 has compeleted execution and Thread 3 is mid execution. Then current value of mutex->turn is 2. When Thread 1 runs >lock(&mutex), its myturn will be calculated now. before the FetchAndAdd runs, the value in mutex->ticket is 3.
// Why? you may ask - see this. initially it was 0. T1 asked for myturn, it got 0, mutex->ticket increased to 1. Then T2 asked for myturn, it got 1, mutex->ticket increased to 2. Then T3 asked for myturn, it got 2, mutex->ticket increased to 3.
//  So, when FetchAndAdd runs now, mutex->ticket increases to 4, but the FetchAndAdd call returns the old value, i.e, 3. So myturn for T1 now is 3. turn is still 2.
// Why? you may ask - When T1 released the lock mutex->turn increased to 1 from 0, when T2 released the lock mutex->turn increased to 2 from 1. T3 is running and is yet to release. So mutex->turn is 2.

// Now, with the myturnvalue T1 got (3), it does the while loop check
// while(mutex->turn != myturn), while (2 != 3) -> loop condition met. So, T1 goes an spin-waits.