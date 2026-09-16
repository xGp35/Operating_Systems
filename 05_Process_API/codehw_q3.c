/*
Write another program using fork(). The child process should
print “hello”; the parent process should print “goodbye”. You should
try to ensure that the child process always prints first; can you do
this without calling wait() in the parent?

Ans: Simple approach: sleep()
*/

#include <stdio.h>
#include <unistd.h>

int main() {
    int rc = fork();

    if (rc < 0) {
        return 1;
    } 
    else if (rc == 0) {
        // Child
        printf("hello\n");
    } 
    else {
        // Parent
        sleep(1);
        printf("goodbye\n");
    }

    return 0;
}

/*
But is this a guarantee?

No.

sleep(1) says:

"Parent, don't run for at least 1 second."

It does not say:

"Child, definitely run and print before the parent."

For example, the child could theoretically be descheduled for more than a second.

So if the question literally asks:

"Can you ensure that the child always prints first without wait()?"

Then sleep() isn't a proper synchronization mechanism.

You need some form of synchronization between the processes, such as a pipe, signal, semaphore, etc.


#include <stdio.h>
#include <unistd.h>

int main() {
    int p[2];
    pipe(p);

    int rc = fork();

    if (rc == 0) {
        // Child
        printf("hello\n");

        // Tell parent that hello has been printed
        write(p[1], "x", 1);
    } 
    else {
        // Parent
        char buf;
        read(p[0], &buf, 1);

        // Only reaches here after child writes
        printf("goodbye\n");
    }

    return 0;
}

Now the ordering is:

Child                         Parent
  |                              |
  | print "hello"                |
  |                              |
  | write() ──────────────────>  |
  |                              | read() completes
  |                              |
  |                              | print "goodbye"

So:

hello
goodbye

is guaranteed.

*/