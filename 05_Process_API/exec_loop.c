#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int height = 51;

int main (int argc, char *argv[])
{
    printf("selfExec: My pid is %d.\n", getpid());
    printf("  height: %d\n", height);
    height--;

    if (argc != 2) {
        printf("Pass 1 argument as an integer\n");
        exit(1);
    }

    int n = atoi(argv[1]);
    printf("%d...\n", n);

    if (n != 0) {
        char nMinus1[12];
        sprintf(nMinus1, "%d", n-1);
        // printf("Arg to pass %s\n", nMinus1);
        execl(argv[0], argv[0], nMinus1, NULL);
    }

    printf("Farewell cruel world!\n");

    return 0;
}