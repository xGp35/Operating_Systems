//how many bytes of the heap are free?

int bytesLeft = MAX_HEAP_SIZE;

//need lock and condition too
cond_t c;
mutex_t  m;
void* allocate(int size) {
    Pthread_mutex_lock(&m);
    while (bytesLeft < size) {
        Pthread_cond_wait(&c, &m);
    }
    void * ptr = ... ; // get mem from heap
    bytesLeft -= size;
    Pthread_mutex_unlock(&m);
    return ptr; 
} 

void free(void* ptr, int size) {
    Pthread_mutex_lock(&m);
    //return some memory to Heap
    bytesLeft += size;
    Pthread_cond_broadcast(&c); //Pthread_cond_signal(&c) will wake only one thread, but we need to wake all threads;
    Pthread_mutex_unlock(&m);
}

// But most of the time we'd not need broadcast. See for bugs that can resolve using signal.
