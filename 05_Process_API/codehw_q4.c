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

/*
There are six variants

execl()
execle()
execlp()

execv()
execve()
execvp()
execvpe()   ← GNU/Linux extension

The names look confusing, but there is actually a very nice pattern.

First letter: l vs v

This tells you how you provide the arguments.

l = list
execl("/bin/ls", "ls", "-l", "-a", NULL);

Arguments are written individually:

       program
          ↓
execl("/bin/ls", "ls", "-l", "-a", NULL)
                ↑     ↑     ↑
              argv[0] argv[1] argv[2]
v = vector

You give an array of arguments:

char *args[] = {"ls", "-l", "-a", NULL};

execv("/bin/ls", args);

So:

l → list arguments individually

v → vector (array) of arguments

Second letter: p

p means:

Search for the program in PATH.

Without p:

execl("/bin/ls", ...);

You provide the actual path.

With p:

execlp("ls", ...);

You only provide:

"ls"

and exec searches the directories in $PATH.


Third letter: e

e means:

You explicitly provide the environment.

For example:

char *env[] = {
    "MY_VARIABLE=hello",
    NULL
};

execle("/bin/ls", "ls", "-l", NULL, env);

You're saying:

Run /bin/ls

with this environment:
    MY_VARIABLE=hello

Without e, exec inherits the current environment.
*/