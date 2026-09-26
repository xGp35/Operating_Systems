int CompareAndSwap(int* ptr, int expected, int new) {
    int original = *ptr;
    if (original == expected) {
        *ptr = new;
    }
    return original;
}

typedef struct __lock_t { 
    int flag;
} lock_t;

void init(lock_t *mutex) {
    // 0 -> lock is available(free), 1 -> held
    mutex->flag = 0;
}

void lock(lock_t *mutex) {
    while (CompareAndSwap(&mutex->flag, 0, 1) == 1) //Test the flag
        ; //spin-wait (do nothing)
}

void unlock(lock_t *mutex) {
    mutex->flag = 0;
}