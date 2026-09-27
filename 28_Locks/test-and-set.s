.var mutex
.var count

.main
.top	

.acquire
mov  $1, %ax        # move value 1 to ax register.
xchg %ax, mutex     # atomic swap of 1 and mutex (1 was in ax, so swap 1 and variable mutex)
test $0, %ax        # if we get 0 back: lock is free!
jne  .acquire       # if not, try again

# critical section
mov  count, %ax     # get the value at the address
add  $1, %ax        # increment it
mov  %ax, count     # store it back

# release lock
mov  $0, mutex

# see if we're still looping
sub  $1, %bx
test $0, %bx
jgt .top	

halt

# lock acquired via a variable called mutex. Probably a mutex object.
# Q. How is the lock acquire written?
# Q. How about lock release?

# In TestAndSet I take an old_ptr, and new value. We first store the old value pointed to by old pointer in one of the registers. Then put the new value in the address pointed to by old_ptr. Then return the old value to the process.

# However, the fundamental truth is that test-and-set with xchg is genuinely atomic—no scheduling pattern or thread count will corrupt the count variable because the lock correctly enforces mutual exclusion. That's precisely why this instruction exists as a synchronization primitive.