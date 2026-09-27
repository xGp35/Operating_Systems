#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "common_threads.h"

typedef struct __counter_t {
    int value;
    pthread_mutex_t mutex;
} counter_t;

void init(counter_t *c) {
    c->value = 0;
    Pthread_mutex_init(&c->mutex, NULL);

void increment(counter_t *c) {
    Pthread_mutex_lock(&c->mutex);
    c->value++;
    Pthread_mutex_unlock(&c->mutex);
}

void decrement(counter_t *c) {
    Pthread_mutex_lock(&c->mutex);
    c->value--;
    Pthread_mutex_unlock(&c->mutex);
}

int get(counter_t *c) {
    Pthread_mutex_lock(&c->mutex);
    int rc = c->value;
    Pthread_mutex_unlock(&c->mutex);
    return rc;
}