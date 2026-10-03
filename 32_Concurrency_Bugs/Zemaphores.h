#ifndef __Zemaphores_h__
#define __Zemaphores_h__

// Zemaphores: semaphores built from mutex + condition variable.
// Works on both Linux and macOS (unlike POSIX unnamed semaphores
// which macOS has deprecated).
// Based on 07_Zemaphores.c from the OSTEP textbook.

typedef struct __Zem_t {
    int value;
    pthread_cond_t cond;
    pthread_mutex_t lock;
} Zem_t;

// only one thread can call this.
static inline void Zem_init(Zem_t *s, int value) {
    s->value = value;
    Cond_init(&s->cond);
    Mutex_init(&s->lock);
}

static inline void Zem_wait(Zem_t *s) {
    Mutex_lock(&s->lock);
    while (s->value <= 0) {
        Cond_wait(&s->cond, &s->lock);
    }
    s->value--;
    Mutex_unlock(&s->lock);
}

static inline void Zem_post(Zem_t *s) {
    Mutex_lock(&s->lock);
    s->value++;
    Cond_signal(&s->cond);
    Mutex_unlock(&s->lock);
}

#endif // __Zemaphores_h__
