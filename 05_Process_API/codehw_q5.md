Let's make this concrete, because this question is really about **what `wait()` means to the parent and why it behaves differently in the child.**

### 1. Basic program

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
        int wc = wait(NULL);
        printf("Parent: wait() returned %d\n", wc);
        printf("Parent: child has finished\n");
    }

    return 0;
}
```

The important line is:

```c
int wc = wait(NULL);
```

### What does `wait()` do?

When the **parent** calls `wait()`:

```text
Parent                         Child
   |                              |
   | wait()                       |
   |                              |
   |  BLOCKED                     | running
   |                              |
   |                              | exits
   | <----------------------------|
   |                              |
   | wait() returns               |
   ↓
continue
```

So `wait()` basically says:

> **"Parent, stop here until one of your children terminates."**

---

## What does `wait()` return?

This is the interesting part.

If `wait()` succeeds, it returns the **PID of the child that terminated**.

So if:

```text
Child PID = 12345
```

then:

```c
int wc = wait(NULL);
```

will give:

```text
wc == 12345
```

It does **not** return the child's exit status directly.

If you want the child's exit status, you use:

```c
int status;
int wc = wait(&status);
```

and then inspect `status` using macros such as `WIFEXITED()` and `WEXITSTATUS()`.

For this OSTEP question, the main thing to remember is:

> **`wait()` returns the PID of the child that was successfully waited for.**

If there are no children to wait for, it returns `-1`.

---

# 2. What happens if the child calls `wait()`?

Now change the program:

```c
else if (rc == 0) {
    // Child
    int wc = wait(NULL);
    printf("Child: wait() returned %d\n", wc);
}
```

The child doesn't have any children of its own.

Remember:

```text
           fork()
          /      \
      Parent     Child
         |          |
      has child   has NO children
```

`wait()` means:

> "Wait for one of **my children** to terminate."

But the child has no children.

Therefore:

```c
wait(NULL)
```

**immediately returns `-1`** and sets `errno` to `ECHILD` ("No child processes").

So:

```text
Parent
  |
  | wait()
  ↓
blocks until Child exits


Child
  |
  | wait()
  ↓
returns -1 immediately
because Child has no children
```

### The mental model

The most useful way to remember this is:

> **`wait()` isn't "wait for another process." It's "wait for one of *my child processes*."**

That's why:

```text
Parent → wait() → waits for Child
Child  → wait() → has nobody to wait for
```

And this also connects nicely to `fork()`:

```text
fork()
  ↓
Parent creates Child
  ↓
Parent can wait for Child
```

The relationship created by `fork()` is what makes the `wait()` relationship possible.
