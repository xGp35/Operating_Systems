#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int pipefd[2]; // could have been arr[2] as well

    // pipefd[0] = read end
    // pipefd[1] = write end
    pipe(pipefd);

    // Say, FD table already has 0–5, so pipe created by kernel in prev step gets 6 and 7 — Yes. Then we we assign this pipefd array to the actual pipe, arr[0] becomes 6 (the read end), and arr[1] becomes 7 (the write end). Lowest available, as always.

    int child1 = fork();

    if (child1 == 0) {
        // Child 1: stdout → pipe

        close(pipefd[0]);  // Don't need read end
        //close(pipefd[0]) closes fd 6 in the child's fd table only. The parent still has fd 6 open. Now child 1 only has the write end (fd 7).

        dup2(pipefd[1], STDOUT_FILENO);  

        close(pipefd[1]);

        printf("Hello from child 1!\n");

        return 0;
    }

    int child2 = fork();

    if (child2 == 0) {
        // Child 2: pipe → stdin

        close(pipefd[1]);  // Don't need write end

        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);

        char buffer[100];

        int n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);

        if (n > 0) {
            buffer[n] = '\0';
            printf("Child 2 received: %s", buffer);
        }

        return 0;
    }

    // Parent doesn't need the pipe
    close(pipefd[0]);
    close(pipefd[1]);

    wait(NULL);
    wait(NULL);

    return 0;
}