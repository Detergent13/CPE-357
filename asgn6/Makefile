CC = gcc
CFLAGS = -Wall -pedantic -g

mush2: mush2.o
	gcc -g -Wall -L ./Mush/lib64 -o mush2 mush2.o -lmush
mush2.o: mush2.c
	gcc -g -c -Wall -I ./Mush/include -o mush2.o mush2.c
all: mush
test: mush
	./mush
clean:
