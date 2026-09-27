.var mutex
.var count

.main
.top	

.acquire
mov  mutex, %ax
test $0, %ax
jne .acquire
mov  $1, %ax        
xchg %ax, mutex     # atomic swap of 1 and mutex
test $0, %ax        # if we get 0 back: lock is free!
jne .acquire        # if not, try again

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

/*
TTAS saves 1 instruction per failed spin (3 vs 4), but pays 3 extra instructions every time it actually acquires (7 vs 4). In most runs, that acquire penalty dominates.

Why TTAS Exists (And Why It Can't Show Its Advantage Here)
The real-world advantage of test-and-test-and-set has nothing to do with instruction count. It's about bus traffic on real multicore hardware:

mov mutex, %ax (plain read) can be served from the CPU's local cache. Cost: ~1 cycle. No bus traffic. Other CPUs aren't disturbed.
xchg %ax, mutex (atomic swap) requires exclusive ownership of the cache line. The CPU must broadcast an invalidation to every other core, wait for acknowledgments, and only then perform the swap. Cost: ~100-500 cycles of bus traffic.
When 4 threads are spinning with plain test-and-set, all 4 are hammering xchg every iteration -- that's 4 streams of cache invalidation messages flooding the bus. The bus becomes the bottleneck.

With test-and-test-and-set, those 4 threads spin on local cache reads (mov mutex, %ax). Zero bus traffic while the lock is held. Only when the lock is released (and they all see mutex=0) do they attempt the expensive xchg.

This simulator has no cache. No bus. No MESI protocol. Every memory access -- read or write, atomic or not -- costs exactly one instruction tick. The entire premise of TTAS (cheap reads vs. expensive atomic writes) collapses to nothing. You're left with the raw instruction count, where TTAS is worse because of the extra peek-before-swap phase.

The One-Sentence Version
Test-and-test-and-set optimizes for a cost model (cache coherence traffic) that this simulator doesn't have. In a flat-cost-per-instruction simulator, the extra "peek first" phase is pure overhead.
*/