#ifndef __concurrent_linked_list_h__
#define __concurrent_linked_list_h__

#include <pthread.h>

// basic node structure
typedef struct __node_t {
    int key;
    struct __node_t *next;
} node_t;

// basic Linked List structure
typedef struct __list_t {
    node_t *head;
    pthread_mutex_t lock;
} list_t;

void List_Init(list_t *L);
int  List_Insert(list_t *L, int key);
int  List_Lookup(list_t *L, int key);
int  List_Remove(list_t *L, int key);

#endif // __concurrent_linked_list_h__
