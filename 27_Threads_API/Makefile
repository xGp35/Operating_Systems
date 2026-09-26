FLAGS = -Wall -pthread -g

all: main-race.out main-deadlock.out main-deadlock-global.out main-signal.out main-signal-cv.out

clean:
	rm -f main-race.out main-deadlock.out main-deadlock-global.out main-signal.out main-signal-cv.out

main-race.out: main-race.c common_threads.h
	gcc -o main-race.out main-race.c $(FLAGS)

main-deadlock.out: main-deadlock.c common_threads.h
	gcc -o main-deadlock.out main-deadlock.c $(FLAGS)

main-deadlock-global.out: main-deadlock-global.c common_threads.h
	gcc -o main-deadlock-global.out main-deadlock-global.c $(FLAGS)

main-signal.out: main-signal.c common_threads.h
	gcc -o main-signal.out main-signal.c $(FLAGS)

main-signal-cv.out: main-signal-cv.c common_threads.h
	gcc -o main-signal-cv.out main-signal-cv.c $(FLAGS)