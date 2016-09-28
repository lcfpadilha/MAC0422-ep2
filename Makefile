CC=gcc
CFLAGS=-Wall -pedantic -O2 -g
THREADS=-lpthread

all: ep2

ep2: ep2.o
	${CC} ${CFLAGS} -o ep2 $^ ${THREADS}

ep2.o: ep2.c

clean:
	rm ep2 *.o
