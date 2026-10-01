#include <semaphore.h>
sem_t s;
sem_init(&s, 0, 1);

int sem_wait(sem_t *s) {
    //decrement the value of semaphore s by 1
    //wait if the value of semaphore is negative.
}

int sem_post(sem_t *s) {
    //increment the value of semaphore s by 1
    // if there are 1 or more threads waiting, wake one.
}