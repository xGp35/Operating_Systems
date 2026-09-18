#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <memory_MB>\n", argv[0]);
        return 1;
    }

    int mb = atoi(argv[1]);

    // Allocate the requested amount of memory.
    size_t bytes = (size_t)mb * 1024 * 1024;
    int *array = malloc(bytes);

    if (array == NULL) {
        perror("malloc");
        return 1;
    }

    // Number of integers we can fit in the allocated memory.
    long num_elements = (mb * 1024L * 1024L) / sizeof(int);

    printf("Allocated %d MB\n", mb);

    // Continuously touch every element.
    while (1) {
        for (long i = 0; i < num_elements; i++) {
            array[i] += 1;
        }
    }

    free(array);
    return 0;
}