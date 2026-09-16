/*
Write a program that calls fork() and then calls some form of
exec()to run the program /bin/ls. See if you can try all of the
variants of exec(), including (on Linux) execl(), execle(),
execlp(), execv(), execvp(), and execvpe(). Why do
you think there are so many variants of the same basic call?
*/

#include <stdio.h>
#include <unistd.h>

int main() {
    int rc = fork();

    if (rc < 0) {
        perror("fork");
        return 1;
    }

    if (rc == 0) {
        // Child
        execl("/bin/ls", "ls", "-l", NULL);

        // Only reached if exec fails
        perror("execl");
        return 1;
    }

    // Parent
    printf("Parent: child created\n");

    return 0;
}