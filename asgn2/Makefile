CC = gcc
CFLAGS = -Wall -pedantic -g

fw: fw.c node.h
	$(CC) $(CFLAGS) -o fw fw.c 
all: fw
test: fw
	./fw
clean:
	rm fw.err.*
	rm fw.output*
	rm reference.output*
