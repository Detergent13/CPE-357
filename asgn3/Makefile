CC = gcc
CFLAGS = -Wall -pedantic -g

all: hencode hdecode
hencode: hencode.c util.h util.o nodes.h
	$(CC) $(CFLAGS) -o hencode hencode.c util.o
hdecode: hdecode.c util.h util.o nodes.h
	$(CC) $(CFLAGS) -o hdecode hdecode.c util.o
util.o: util.c nodes.h
	$(CC) $(CFLAGS) -c util.c
clean:
