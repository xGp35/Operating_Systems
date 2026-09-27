#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "common_threads.h"
#include "concurrent_linked_list.h"

// Do i need all these headers?
// I am using List_Insert, which was defined in another file. what should i do about it? Can I use it naturally as I have done here? Or do I need to create a header file?

#define  BUCKETS (101) // Why put this in brackets?-  Defensive macro hygiene. macros like #define X 2+3 would break in 5 * X (gives 5*2+3 = 13 instead of 25). Always wrapping in parens is a "just always do it" C convention for macros.

typedef struct __hash_t {
    list_t lists[BUCKETS]; // This is an array of linked lists. The type of linked list is list_t. So a bunch of headers to linked lists in an array.
} hash_t;

void Hash_Init(hash_t *H) {
    int i;
    for (i = 0; i < BUCKETS; i++) {
        List_Init(&H->lists[i]); // Initialize 101 linked list heads which will serve as our carriers for HashValues
    } // Also because List_Init is  already defining the locks, we don't worry about that here.
}

int Hash_Insert(hash_t *H, int key) {
    return List_Insert(&H->lists[key % BUCKETS], key);
}

int Hash_Lookup(hash_t *H, int key){
    return List_Lookup(&H->lists[key % BUCKETS], key);
}

/*
hash_t H
┌─────────────────────────────────────────────────────────┐
│  lists[0]          lists[1]          lists[2]     ...  lists[100]  │
│ ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐│
│ │head ─────────>│head ─────────>│head=NULL  │    │head ─────────>│
│ │lock      │    │lock      │    │lock      │    │lock      ││
│ └──────────┘    └──────────┘    └──────────┘    └──────────┘│
│      │               │                                │     │
│      ▼               ▼                                ▼     │
│   [key=5]        [key=102]                        [key=201] │
│      │               │                                      │
│      ▼               ▼                                      │
│   [key=106]      [key=304]                                  │
└─────────────────────────────────────────────────────────────┘
*/