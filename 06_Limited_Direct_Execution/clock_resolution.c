#include <stdio.h>
#include <time.h>

int main() {
    struct timespec res;

    clock_getres(CLOCK_MONOTONIC, &res);

    printf("Resolution: %ld ns\n",
           res.tv_sec * 1000000000L + res.tv_nsec);
    // Take the seconds portion, convert it to nanoseconds, then add the nanosecond portion.

    return 0;
}

/*
struct timespec {
    time_t tv_sec;   // seconds
    long   tv_nsec;  // nanoseconds
};
*/