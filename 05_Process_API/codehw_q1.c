/*
Write a program that calls fork(). Before calling fork(), have the main process access a variable (e.g., x) and set its value to some- thing (e.g., 100). What value is the variable in the child process? What happens to the variable when both the child and parent change the value of x?
*/

#include <stdio.h>
#include <unistd.h>

int main() {
    int x = 100;

    printf("Before fork: x = %d\n", x);

    int rc = fork();

    if (rc < 0) {
        // fork failed
        return 1;
    } 
    else if (rc == 0) {
        // Child
        printf("Child: x = %d\n", x);
        x = 200;
        printf("Child after changing x: x = %d\n", x);
    } 
    else {
        // Parent
        printf("Parent: x = %d\n", x);
        x = 300;
        printf("Parent after changing x: x = %d\n", x);
    }

    return 0;
}

/*
the OS creates the child with a copy of the parent's address space:

             fork()
               |
       +-------+-------+
       |               |
    Parent           Child
     x=100            x=100

So the child initially sees x = 100.

Then suppose:

// child
x = 200;

// parent
x = 300;

You get:

Parent's memory        Child's memory
      x = 300               x = 200

Changing x in one process does not change x in the other process.
*/