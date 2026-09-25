#include <stdio.h>
#include <pthread.h>

typedef struct {
    int a;
    int b;
} myarg_t;

void *mythread(void* arg) {
    myarg_t *args = (myarg_t *) arg;
    printf("%d %d\n", args->a, args->b);
    return NULL;
}
// pthread_create returns an int — 0 on success, and a nonzero error number on failure
int main(int argc, char* argv[]) {
    pthread_t p;
    myarg_t funny = {10, 20};

    int rc = pthread_create(&p, NULL, mythread, &funny);
}