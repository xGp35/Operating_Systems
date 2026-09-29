
BINARIES = main-two-cvs-while.out main-two-cvs-if.out main-one-cv-while.out main-two-cvs-while-extra-unlock.out
HEADERS = common.h common_threads.h main-header.h main-common.c pc-header.h

all: $(BINARIES)

clean:
	rm -f $(BINARIES)

main-one-cv-while.out: main-one-cv-while.c $(HEADERS)
	gcc -o main-one-cv-while.out main-one-cv-while.c -Wall -pthread

main-two-cvs-if.out: main-two-cvs-if.c $(HEADERS)
	gcc -o main-two-cvs-if.out main-two-cvs-if.c -Wall -pthread

main-two-cvs-while.out: main-two-cvs-while.c $(HEADERS)
	gcc -o main-two-cvs-while.out main-two-cvs-while.c -Wall -pthread

main-two-cvs-while-extra-unlock.out: main-two-cvs-while-extra-unlock.c $(HEADERS)
	gcc -o main-two-cvs-while-extra-unlock.out main-two-cvs-while-extra-unlock.c -Wall -pthread


