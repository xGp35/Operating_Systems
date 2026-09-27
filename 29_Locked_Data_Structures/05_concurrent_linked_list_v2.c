#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "common_threads.h"

// basic node structure
typedef struct __node_t {
    int key;
    struct __node_t * next; // * after a "type", so next is a pointer that points to a data structure of same type
} node_t;

// basic Linked List structure
typedef struct __list_t {
    node_t * head;
    pthread_mutex_t lock;
} list_t;  // Why have  2 names __list_t and list_t -> authors are mad.

void List_Init(list_t *L) {
    L->head = NULL;
    Pthread_mutex_init(&L->lock, NULL);
}

// This inserts at the beginning of a list? - Yes. O(1) insert.
int List_Insert(list_t *L, int key) {
    // Assume malloc is already thread safe.
    node_t *new = malloc(sizeof(node_t));
    if (new == NULL) {
        perror("malloc");
        return -1; // fail
    }
    new->key = key; // The new node is a local variable that no other thread knows about yet, so setting new->key is also safe without the lock.
    // Lock needs to be held only when updating L. L is a shared data structure that other threads might also be accessing
    Pthread_mutex_lock(&L->lock);
    new->next = L->head;
    L->head = new;
    Pthread_mutex_unlock(&L->lock);
    return 0; //success
}

int List_Lookup(list_t *L, int key) {
    int rv = -1; // Why is rv = -1 set before the lock? Because rv is a local variable on the stack. no other thread can see it or touch it. There's no concurrency concern with setting it.
    Pthread_mutex_lock(&L->lock);
    node_t *curr = L->head;
    while (curr) {
        if (curr->key == key) {
            rv = 0;
            break; // notice we are not returning from here anymore.
        }
        curr = curr->next;
    }
    Pthread_mutex_unlock(&L->lock);
    return rv; //now both success and failure.
}
