## Q6
Yes. The easiest way to understand `waitpid()` is to start with the limitation of `wait()`.

### The problem with `wait()`

Suppose your parent creates **two children**:

```text
             Parent
             /    \
        Child A   Child B
```

Now the parent does:

```c
wait(NULL);
```

The meaning is essentially:

> "Wait for **any one** of my children to finish."

But what if you specifically want to wait for **Child B**?

That's where `waitpid()` comes in.

---

## Modification of your previous program

Your previous code had:

```c
int wc = wait(NULL);
```

Change that to:

```c
int wc = waitpid(rc, NULL, 0);
```

Full program:

```c
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
        int wc = waitpid(rc, NULL, 0);

        printf("Parent: waitpid() returned %d\n", wc);
        printf("Parent: child has finished\n");
    }

    return 0;
}
```

Remember that in the parent:

```c
rc
```

contains the **PID of the child**.

So if:

```text
rc = 5001
```

then:

```c
waitpid(rc, NULL, 0);
```

means:

> **"Wait until child 5001 terminates."**

---

## Compare them

### `wait()`

```c
wait(NULL);
```

means roughly:

```text
"Give me whichever child finishes first."
```

### `waitpid()`

```c
waitpid(5001, NULL, 0);
```

means:

```text
"Wait specifically for child 5001."
```

That's the main reason `waitpid()` exists.

---

## Why is this useful?

Imagine:

```text
                    Parent
                  /    |    \
                 /     |     \
              Child A Child B Child C
```

Suppose:

```text
Child A → takes 10 seconds
Child B → takes 2 seconds
Child C → takes 5 seconds
```

If you do:

```c
wait(NULL);
```

you don't get to choose. The first child to terminate gets collected.

But you might have a reason to specifically wait for Child A:

```c
waitpid(child_a_pid, NULL, 0);
```

Now the parent waits specifically for A.

---

### One more thing about the arguments

You'll see:

```c
waitpid(pid, &status, options);
```

There are three pieces:

```text
waitpid(
    pid,       ← WHICH child?
    &status,   ← WHAT happened to it?
    options    ← HOW should I wait?
)
```

For our simple example:

```c
waitpid(rc, NULL, 0);
```

means:

* `rc` → wait for this specific child
* `NULL` → I don't care about its exit status
* `0` → wait normally/block until it terminates

So the key mental model is:

> **`wait()` = "wait for a child."**
> **`waitpid()` = "wait for *this particular* child (and optionally control how you wait)."**


## Q7

