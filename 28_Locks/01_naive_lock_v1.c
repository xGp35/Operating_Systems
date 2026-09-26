#include <stdio.h>

#include "common_threads.h"

typedef struct __lock_t { 
    int flag;
} lock_t;

void init(lock_t *mutex) {
    // 0 -> lock is available(free), 1 -> held
    mutex->flag = 0;
}

void lock(lock_t *mutex) {
    while (mutex->flag == 1) //Test the flag
        ; //spin-wait (do nothing)
    mutex-> flag = 1;        // no SET it !
}

void unlock(lock_t *mutex) {
    mutex->flag = 0;
}