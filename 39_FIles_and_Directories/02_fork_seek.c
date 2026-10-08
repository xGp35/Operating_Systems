#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>

int main(int argc, char* argv[]) {
    int fd = open("file.txt", O_RDONLY);
    assert(fd >= 0);
    int rc = fork(); // What was returned by fork again? See below

    if (rc ==0) {
        rc = lseek(fd, 10, SEEK_SET); // sets the offset in child process to 10
        printf("child: offset %d\n", rc);
    } else if (rc > 0) {
        (void) wait(NULL);
        printf("parent: offset %d\n", (int) lseek(fd, 0, SEEK_CUR)); // Stay in the curre offset.
    }
    return 0;
}

/*
What was returned by fork again?
- In the parent process: returns the PID of the child (a positive integer).
- In the child process: returns 0.
- On failure: returns -1 (no child created, errno set, e.g. EAGAIN or ENOMEM).
*/