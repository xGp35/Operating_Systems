int TestAndSet(int *old_ptr, int new) {
    int old = *old_ptr;   // fetch old value at old_ptr
    *old_ptr = new;       // store 'new' into old_ptr
    return old;           // return the old value
}

typedef struct __lock_t { 
    int flag;
} lock_t;

void init(lock_t *mutex) {
    // 0 -> lock is available(free), 1 -> held
    mutex->flag = 0;
}

void lock(lock_t *mutex) {
    while (TestAndSet(&mutex->flag, 1) == 1) //Test the flag
        yield(); //deschedule itself. Go from running to runnable/ready state.
}

void unlock(lock_t *mutex) {
    mutex->flag = 0;
}