CC = gcc
CFLAGS = -Wall -pedantic -g

timeit: timeit.c
	$(CC) $(CFLAGS) -o timeit timeit.c
all: timeit
test: timeit
	./timeit
clean:
