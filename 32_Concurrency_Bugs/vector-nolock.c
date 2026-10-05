#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "common_threads.h"

#include "main-header.h"
#include "vector-header.h"

// taken from https://en.wikipedia.org/wiki/Fetch-and-add
// using GCC/Clang built-in for portability (original x86 asm doesn't work on ARM64)
int fetch_and_add(int *variable, int value) {
    return __sync_fetch_and_add(variable, value);
}

void vector_add(vector_t *v_dst, vector_t *v_src) {
    int i;
    for (i = 0; i < VECTOR_SIZE; i++) {
	    fetch_and_add(&v_dst->values[i], v_src->values[i]);
    }
}

void fini() {}


#include "main-common.c"

