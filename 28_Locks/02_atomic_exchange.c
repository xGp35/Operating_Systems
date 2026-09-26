int TestAndSet(int *old_ptr, int new) {
    int old = *old_ptr;   // fetch old value at old_ptr
    *old_ptr = new;       // store 'new' into old_ptr
    return old;           // return the old value
}

// Took the "address of 2"(old_ptr) and value 3(new)
// extracted the value pointed to by old_ptr (2)
// put the value 3 in the "address of 2". Now, the address pointed to by old_ptr holds 3 instead of 2.
// return 2