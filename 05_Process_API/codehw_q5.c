#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int rc = fork();

    if (rc < 0) {
        return 1;
    }
    else if (rc == 0) {
        // Child
        printf("Child: hello\n");
        sleep(2);
        printf("Child: exiting\n");
    }
    else {
        // Parent
        int wc = wait(NULL);
        printf("Parent: wait() returned %d\n", wc);
        printf("Parent: child has finished\n");
    }

    return 0;
}