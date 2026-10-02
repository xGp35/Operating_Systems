
// When a philosopher wishes to refer to the fork on their left they call left(p)
int left(int p) {
    return p;
}
// Fork on the right of philospher is referred to by calling right(p)
int right(int p) {
    return (p+1)%5;
}

sem_t forks[5];
// Initialize each semaphore in this array to a value of 1
int i;
for(i=0; i<5; i++) {
    sem_init(&forks[i], 0, 1);
}

void getfork(int p) {
    if (p == 4) {
        sem_wait(forks[right(p)]);
        sem_wait(forks[left[p]]);
    } else {
        sem_wait(forks[left[p]]);
        sem_wait(forks[right(p)]);
    }
}

void putfork(int p) {
    sem_wait(forks[left[p]]);
    sem_wait(forks[right(p)]);
}

//So this is the classic Dining Philosophers problem, and the solution is relatively elegant and simple. The only thing that we are doing here is we are changing the order of fork picking of the last philosopher in the circle, and this will indeed prevent the deadlock that we have. Let's say, like in the normal deadlock scenario, where this extra last deadlock, sorry, last philosopher condition was not established. In that case, everyone picks the fork on their left but are unable to pick the fork on their right. That's why it's a deadlock. But in case we set the last philosopher to pick the first fork from his right, then what will happen is it will check that that fork is already taken. It has already a lock associated with it, and it will go to sleep. When it goes to sleep, then the second last philosopher would be free to acquire both the spoons. So once the second last is done with his work, then it can release both of them, and then consecutively all of them can release after each one is done, and then the last one will also get. There can be starvation, but no deadlock.