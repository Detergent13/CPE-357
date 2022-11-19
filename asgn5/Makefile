CC = gcc
CFLAGS = -Wall -pedantic -g

mytalk: mytalk.o
	$(CC) $(CFLAGS) -L ./Talk/lib64 -o mytalk mytalk.o -ltalk -lncurses
mytalk.o: mytalk.c
	$(CC) $(CFLAGS) -c -I ~./Talk/include mytalk.c
all: mytalk
test: mytalk
	./mytalk
clean:
