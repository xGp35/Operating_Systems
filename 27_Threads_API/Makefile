
FLAGS = -Wall -pthread -g

all: main-race main-deadlock main-deadlock-global main-signal main-signal-cv

clean:
	rm -f main-race.out main-deadlock.out main-deadlock-global.out main-signal.out main-signal-cv.out

main-race: main-race.c common_threads.h
	gcc -o main-race.out main-race.c $(FLAGS)

main-deadlock: main-deadlock.c common_threads.h
	gcc -o main-deadlock.out main-deadlock.c $(FLAGS)

main-deadlock-global: main-deadlock-global.c common_threads.h
	gcc -o main-deadlock-global.out main-deadlock-global.c $(FLAGS)

main-signal: main-signal.c common_threads.h
	gcc -o main-signal.out main-signal.c $(FLAGS)

main-signal-cv: main-signal-cv.c common_threads.h
	gcc -o main-signal-cv.out main-signal-cv.c $(FLAGS)
