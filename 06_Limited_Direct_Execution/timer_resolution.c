#include <stdio.h>
#include <sys/time.h>

int main() {
    struct timeval start, end;

    gettimeofday(&start, NULL);
    gettimeofday(&end, NULL);

    long long elapsed =
        (end.tv_sec - start.tv_sec) * 1000000LL +
        (end.tv_usec - start.tv_usec);

    printf("Time between two calls: %lld microseconds\n", elapsed);

    return 0;
}