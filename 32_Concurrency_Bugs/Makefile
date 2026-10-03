
ALL = vector-deadlock.out vector-global-order.out vector-try-wait.out vector-avoid-hold-and-wait.out vector-nolock.out
COMMON = vector-header.h main-common.c main-header.h common.h common_threads.h

all: $(ALL)

clean:
	rm -f $(ALL) *~

vector-deadlock.out: vector-deadlock.c $(COMMON)
	gcc -o vector-deadlock.out vector-deadlock.c -Wall -pthread -O

vector-global-order.out: vector-global-order.c $(COMMON)
	gcc -o vector-global-order.out vector-global-order.c -Wall -pthread -O

vector-try-wait.out: vector-try-wait.c $(COMMON)
	gcc -o vector-try-wait.out vector-try-wait.c -Wall -pthread -O

vector-avoid-hold-and-wait.out: vector-avoid-hold-and-wait.c $(COMMON)
	gcc -o vector-avoid-hold-and-wait.out vector-avoid-hold-and-wait.c -Wall -pthread -O

vector-nolock.out: vector-nolock.c $(COMMON)
	gcc -o vector-nolock.out vector-nolock.c -Wall -pthread -O

