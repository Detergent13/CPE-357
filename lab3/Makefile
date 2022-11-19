CC = gcc
CFLAGS = -Wall -pedantic -g

htable: htable.c nodes.h
	$(CC) $(CFLAGS) -o htable htable.c
all: htable
test: htable
	./htable
clean:
