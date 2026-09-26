#include <stdio.h>

#include "common_threads.h"

typedef struct __lock_t { 
    int flag;
} lock_t;

void init(lock_t *mutex) {
    // 0 -> lock is available(free), 1 -> held
    mutex->flag = 0;
}

int LoadLinked(int *ptr) {
    return *ptr;
}

int StoreConditional(int *ptr, int value) {
    if ("no update to *ptr since LL to this addr") { //nothing you can actually write in C goes there — that line is pseudocode standing in for a hardware guarantee that plain C can't express
        *ptr = value;
        return 1;  // success !
    } else {
        return 0;  // failed to update
    }
}

void lock(lock_t *mutex) {
    while (1) {
        while (LoadLinked(&mutex->flag) == 1) {
            ; // spin until it's 0
        }
        if (StoreConditional(&mutex->flag, 1) == 1) {
            return;   // if set-to-1 was success: done
        }  // otherwise: try again in infinite loop.
    }
}

void unlock(lock_t *mutex) {
    mutex->flag = 0;
}