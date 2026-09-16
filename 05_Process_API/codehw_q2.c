/*
Write a program that opens a file (with the open() system call)
and then calls fork()to create a new process. Can both the child
and parent access the file descriptor returned by open()? What
happens when they are writing to the file concurrently, i.e., at the
same time? 
*/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd < 0) {
        perror("open");
        return 1;
    }

    int rc = fork();

    if (rc < 0) {
        perror("fork");
        return 1;
    }
    else if (rc == 0) {
        // Child
        write(fd, "Child\n", 6);
        close(fd);
    }
    else {
        // Parent
        write(fd, "Parent\n", 7);
        close(fd);
    }

    return 0;
}