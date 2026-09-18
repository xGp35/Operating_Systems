#include <stdlib.h>

int main(void) {
    int *p = malloc(100 * sizeof(int));

    if (p == NULL) {
        return 1;
    }

    p[0] = 42;

    return 0;
}