// Locks are Binary Semaphores
sem_t mutex;
sem_init(&mutex, 0, X); //init to X; What should X be?

sem_wait(&mutex) {
    //critical section here
}
sem_post(&mutex)

// Mutex was an object initialized to PTHREAD_MUTEX_INITIALIZER,
// here the initialization of the mutex would be different