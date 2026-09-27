#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "common_threads.h"
#include "concurrent_linked_list.h"

// ---- List_Remove implementation (pairs with the linked list in 05) ----
// Removes the first node with matching key. Uses the single-point-of-return pattern.
int List_Remove(list_t *L, int key) {
    int rv = -1;
    Pthread_mutex_lock(&L->lock);
    node_t *curr = L->head;
    node_t *prev = NULL;
    while (curr) {
        if (curr->key == key) {
            // Found it — unlink the node
            if (prev == NULL) {
                L->head = curr->next; // removing the head node
            } else {
                prev->next = curr->next; // bypass curr
            }
            free(curr);
            rv = 0;
            break;
        }
        prev = curr;
        curr = curr->next;
    }
    Pthread_mutex_unlock(&L->lock);
    return rv;
}

// ---- Hash Table v2: dynamic buckets, remove, resize ----

#define INITIAL_BUCKETS (101)

typedef struct __hash_t {
    list_t *lists;                  // dynamically allocated array (not fixed like v1)
    int num_buckets;
    int total_items;                // approximate count — used for resize decisions
    pthread_mutex_t items_lock;     // protects total_items
    pthread_rwlock_t resize_lock;   // read-write lock: normal ops take read lock, resize takes write lock
} hash_t;

// Why a read-write lock for resize?
// Normal operations (insert, lookup, remove) only need the bucket array to not change out from under them.
// Multiple normal ops can run concurrently — they each grab the READ lock (shared).
// Resize needs exclusive access to swap the entire bucket array — it grabs the WRITE lock.
// This way, resize blocks everything, but normal ops don't block each other.

void Hash_Init(hash_t *H) {
    H->num_buckets = INITIAL_BUCKETS;
    H->total_items = 0;
    H->lists = malloc(sizeof(list_t) * H->num_buckets);
    assert(H->lists != NULL);
    Pthread_mutex_init(&H->items_lock, NULL);
    pthread_rwlock_init(&H->resize_lock, NULL);
    int i;
    for (i = 0; i < H->num_buckets; i++) {
        List_Init(&H->lists[i]);
    }
}

int Hash_Insert(hash_t *H, int key) {
    pthread_rwlock_rdlock(&H->resize_lock);     // shared lock — multiple inserts can happen concurrently
    int bucket = key % H->num_buckets;
    int rv = List_Insert(&H->lists[bucket], key);
    pthread_rwlock_unlock(&H->resize_lock);

    if (rv == 0) {
        Pthread_mutex_lock(&H->items_lock);
        H->total_items++;
        Pthread_mutex_unlock(&H->items_lock);
    }
    return rv;
}

int Hash_Lookup(hash_t *H, int key) {
    pthread_rwlock_rdlock(&H->resize_lock);
    int bucket = key % H->num_buckets;
    int rv = List_Lookup(&H->lists[bucket], key);
    pthread_rwlock_unlock(&H->resize_lock);
    return rv;
}

int Hash_Remove(hash_t *H, int key) {
    pthread_rwlock_rdlock(&H->resize_lock);
    int bucket = key % H->num_buckets;
    int rv = List_Remove(&H->lists[bucket], key);
    pthread_rwlock_unlock(&H->resize_lock);

    if (rv == 0) {
        Pthread_mutex_lock(&H->items_lock);
        H->total_items--;
        Pthread_mutex_unlock(&H->items_lock);
    }
    return rv;
}

// Resize: rehash all elements into a new bucket array of size new_num_buckets.
// This grabs the WRITE lock, which means NO other operation can run during resize.
void Hash_Resize(hash_t *H, int new_num_buckets) {
    pthread_rwlock_wrlock(&H->resize_lock);     // exclusive lock — blocks ALL other operations

    // Allocate and initialize new bucket array
    list_t *new_lists = malloc(sizeof(list_t) * new_num_buckets);
    assert(new_lists != NULL);
    int i;
    for (i = 0; i < new_num_buckets; i++) {
        List_Init(&new_lists[i]);
    }

    // Rehash: walk every node in every old bucket, move it to the correct new bucket.
    // We move nodes directly (no malloc/free) — just re-link pointers.
    for (i = 0; i < H->num_buckets; i++) {
        node_t *curr = H->lists[i].head;
        while (curr) {
            node_t *next = curr->next;  // save next before we re-link curr

            int new_bucket = curr->key % new_num_buckets;
            curr->next = new_lists[new_bucket].head;   // prepend to new bucket
            new_lists[new_bucket].head = curr;

            curr = next;
        }
        // Nodes have been moved, not freed. Just destroy the old bucket's lock.
        pthread_mutex_destroy(&H->lists[i].lock);
    }

    // Swap in the new bucket array
    free(H->lists);
    H->lists = new_lists;
    H->num_buckets = new_num_buckets;

    pthread_rwlock_unlock(&H->resize_lock);
}
