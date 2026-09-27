#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "common_threads.h"

// basic node structure. Not double ended queue
typedef struct __node_t {
    int value;
    struct __node_t * next; // * after a "type", so next is a pointer that points to a data structure of same type
} node_t;

typedef struct __queue_t {
    node_t * head;
    node_t * tail;
    pthread_mutex_t head_lock, tail_lock;
} queue_t;

void Queue_Init(queue_t *q) {
    node_t *tmp = malloc(sizeof(node_t)); // This tmp seems like a dummy node. Is it?
    tmp->next = NULL;
    q->head = q->tail = tmp;
    Pthread_mutex_init(&q->head_lock, NULL);
    Pthread_mutex_init(&q->tail_lock, NULL);
}

void Queue_Enqueue(queue_t *q, int value) { // We enqueue at tail and dequeue at head
    node_t *tmp = malloc(sizeof(node_t));
    assert(tmp != NULL);
    tmp->value = value;
    tmp->next = NULL;

    // We use per node lock, because that's what we concluded from linked list discussion. Locking the whole data structure leads to lock realted waiting and slowness. If possible try to lock individual nodes. In LL that was adding more overhead because there were so many nodes and we had to traverse them. Here there is no traversal overhead, so use per node locks.
    Pthread_mutex_lock(&q->tail_lock);
    q->tail->next = tmp; // assign tmp to current tails next
    q->tail = tmp; // move tail to the last node (tmp)
    Pthread_mutex_unlock(&q->tail_lock);
}

int Queue_Dequeue(queue_t *q, int *value) {
    Pthread_mutex_lock(&q->head_lock);
    node_t *tmp = q->head;
    node_t *new_head = tmp->next;
    if (new_head == NULL) {
        Pthread_mutex_unlock(&q->head_lock);
        return -1; // queue was empty. Wait, why not use the "Single point of return pattern"
    }
    *value = new_head->value;
    q->head = new_head;
    Pthread_mutex_unlock(&q->head_lock);
    free(tmp);
    return 0;
}

/*
You're right that it's different from the DSA pattern you're used to. Let me trace through it concretely.

**Your DSA pattern:** the dummy is permanent, never removed. Real data starts at `dummy->next`.

**This queue:** the dummy **role migrates**. There's always a dummy at the head, but it's not the *same* node forever.

Let me walk through it:

**After Init:**
```
head -> [dummy, val=?] <- tail
```
One node. Both head and tail point to it.

**Enqueue(10):**
```
head -> [dummy, val=?] -> [val=10] <- tail
```
Tail moved forward. Head still on dummy.

**Enqueue(20):**
```
head -> [dummy, val=?] -> [val=10] -> [val=20] <- tail
```

**Dequeue() — first time:**
- `tmp` = head = the original dummy
- `new_head` = `tmp->next` = node(10)
- `*value` = `new_head->value` = **10** (correct!)
- `head` = `new_head` (node with val=10 is now head)
- `free(tmp)` — the original dummy is gone

```
head -> [val=10] -> [val=20] <- tail
```

Now node(10) **is the new dummy**. Its value has already been consumed — it's just sitting there as a placeholder. The real queue content starts at its `->next`.

**Dequeue() — second time:**
- `tmp` = head = node(10) (acting as dummy)
- `new_head` = node(20)
- `*value` = `new_head->value` = **20**
- `head` = node(20)
- `free(tmp)` — node(10) is freed

```
head -> [val=20] <- tail
```

Node(20) is now the dummy. Queue is logically empty.

**So to directly answer your questions:**

"Won't removing the dummy create a problem for the next enqueue?" — No, because there's **always** a dummy at the head. You free the old one but immediately promote the next node to take its place. The invariant is: **the head node is always a dummy whose value has already been consumed (or was never meaningful).** Real data is always at `head->next` onward.

"Is this the same as the DSA pattern?" — Same idea (dummy separates head pointer from real data), different execution. In DSA, the dummy is a fixed sentinel you never touch. Here, the dummy is a **rotating role** — each node becomes the dummy for one dequeue cycle, then gets freed. But the structural invariant is identical: `head` itself is never a "real" element, `head->next` is.
*/