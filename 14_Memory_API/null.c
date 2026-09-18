#include <stdio.h>

int main(void) {
    int *p = NULL;

    printf("About to dereference NULL...\n");
    printf("%d\n", *p);

    return 0;
}