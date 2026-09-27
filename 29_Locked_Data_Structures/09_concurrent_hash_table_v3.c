#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "common_threads.h"
#include "concurrent_linked_list.h"

// ---- List_Remove implementation (pairs with the linked list in 05) ----
int List_Remove(list_t *L, int key) {
    int rv = -1;
    Pthread_mutex_lock(&L->lock);
    node_t *curr = L->head;
    node_t *prev = NULL;
    while (curr) {
        if (curr->key == key) {
            if (prev == NULL) {
                L->head = curr->next;
            } else {
                prev->next = curr->next;
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

// ---- Hash Table v3: automatic resize on insert and remove ----
//
// Resize policy (same idea as Python's dict / your DSA implementation):
//   Grow:   when total_items > num_buckets / 2   (load factor > 0.5)  → double the buckets
//   Shrink: when total_items < num_buckets / 4   (load factor < 0.25) → halve the buckets
//
// Why not shrink at < capacity/2?
//   If you grow at > 1/2 and shrink at < 1/2, one insert+remove cycle near the boundary
//   triggers grow→shrink→grow→shrink endlessly. This is called "thrashing".
//   Shrinking at 1/4 leaves a comfortable gap: you grow at 1/2, but don't shrink
//   until 1/4 — so the table has to lose HALF its elements before it shrinks.

#define INITIAL_BUCKETS (101)
#define MIN_BUCKETS     (INITIAL_BUCKETS)   // never shrink below this

typedef struct __hash_t {
    list_t *lists;
    int num_buckets;
    int total_items;
    pthread_mutex_t items_lock;
    pthread_rwlock_t resize_lock;
} hash_t;

// Forward declaration — resize is a private helper now, not called by the user directly
static void resize_if_needed(hash_t *H);

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
    pthread_rwlock_rdlock(&H->resize_lock);
    int bucket = key % H->num_buckets;
    int rv = List_Insert(&H->lists[bucket], key);
    pthread_rwlock_unlock(&H->resize_lock);

    if (rv == 0) {
        Pthread_mutex_lock(&H->items_lock);
        H->total_items++;
        Pthread_mutex_unlock(&H->items_lock);

        resize_if_needed(H);    // might grow the table
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

        resize_if_needed(H);    // might shrink the table
    }
    return rv;
}

// ---- The actual resize logic ----
// Called after every successful insert or remove.
// Grabs the write lock, re-checks the condition (because another thread might have
// already resized between our check and getting the lock), then rehashes if still needed.
static void resize_if_needed(hash_t *H) {
    int items = H->total_items;         // approximate read — fine for a heuristic check
    int buckets = H->num_buckets;

    int new_num_buckets = 0;

    if (items > buckets / 2) {
        new_num_buckets = buckets * 2;                          // grow
    } else if (items < buckets / 4 && buckets > MIN_BUCKETS) {
        new_num_buckets = buckets / 2;                          // shrink
        if (new_num_buckets < MIN_BUCKETS) {
            new_num_buckets = MIN_BUCKETS;                      // floor
        }
    }

    if (new_num_buckets == 0) return;   // no resize needed

    // Now take the exclusive lock and re-check.
    // Between our check above and acquiring this lock, another thread may have already resized.
    pthread_rwlock_wrlock(&H->resize_lock);

    // Re-check under the write lock — the world may have changed
    items = H->total_items;
    buckets = H->num_buckets;
    int still_needed = 0;
    if (items > buckets / 2) {
        new_num_buckets = buckets * 2;
        still_needed = 1;
    } else if (items < buckets / 4 && buckets > MIN_BUCKETS) {
        new_num_buckets = buckets / 2;
        if (new_num_buckets < MIN_BUCKETS) new_num_buckets = MIN_BUCKETS;
        still_needed = 1;
    }

    if (!still_needed) {
        pthread_rwlock_unlock(&H->resize_lock);
        return;     // another thread already resized — nothing to do
    }

    // Allocate new bucket array
    list_t *new_lists = malloc(sizeof(list_t) * new_num_buckets);
    assert(new_lists != NULL);
    int i;
    for (i = 0; i < new_num_buckets; i++) {
        List_Init(&new_lists[i]);
    }

    // Rehash: move nodes directly — no extra malloc/free
    for (i = 0; i < H->num_buckets; i++) {
        node_t *curr = H->lists[i].head;
        while (curr) {
            node_t *next = curr->next;

            int new_bucket = curr->key % new_num_buckets;
            curr->next = new_lists[new_bucket].head;
            new_lists[new_bucket].head = curr;

            curr = next;
        }
        pthread_mutex_destroy(&H->lists[i].lock);
    }

    // Swap
    free(H->lists);
    H->lists = new_lists;
    H->num_buckets = new_num_buckets;

    pthread_rwlock_unlock(&H->resize_lock);
}
