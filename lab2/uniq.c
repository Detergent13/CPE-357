#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *read_long_line(FILE *file);
int peek(FILE *file);

int main(int argc, char *argv[]) {
	FILE *file;
	char *lastLine;
	char *currentLine;
		
	if(argc != 2) {
		printf("Wrong number of args!");
		exit(1);
	}

	file = fopen(argv[1], "r");
	
	if(peek(file) == EOF) {
		return 0; /* catch empty file */
	}

	lastLine = read_long_line(file); /* load first line */

	if(peek(file) == EOF) {
		puts(lastLine); /* catch one-liner */ 
		return 0;
	}

	currentLine = read_long_line(file); /* load second line */

	puts(lastLine); /* first line will always print if it exists */

	while(peek(file) != EOF) {
		if(strcmp(lastLine, currentLine) != 0) {
			/* write line if it's not the same as the last line */
			puts(currentLine); 
		}
		/* read new line into current and move old current to last */
		lastLine = currentLine;
		currentLine = read_long_line(file);
	}
	
	/* Catch the last line- it's ignored otherwise. */
	if(strcmp(lastLine, currentLine) != 0) {
		puts(currentLine);
	}
	/* This is a bit janky- but I don't see a better way. */

	return 0;
}

/* Reads an arbitrarily long newline-terminated line from the input file */
char *read_long_line(FILE *file) {
	int lineLength = 64;
	char *buffer = (char *)malloc(sizeof(char) * lineLength);
	char ch;
	int count = 0;
	char *line;

	if(buffer == NULL) {
		printf("Something went wrong with mallocing!");
		exit(1);
	}

	ch = getc(file);

	while((ch != '\n') && (ch != EOF)) {
		if(count >= lineLength) {
			lineLength *= 2; /* double acceptable line length */
			buffer = realloc(buffer, lineLength);

			if(buffer == NULL) {
				printf("Something went wrong with reallocing!");
				exit(1);
			}
		}

		buffer[count] = ch;
		count++;

		ch = getc(file);
	}

	buffer[count] = '\0';
	line = (char *)malloc(sizeof(char) * (count + 1));
	strncpy(line, buffer, (count + 1));
	free(buffer);
	return line;
}

/* Just a handy wrapper to peek at the current char w/o disturbing it */
int peek(FILE *file) {
	int ch;
	
	ch = fgetc(file);
	ungetc(ch, file);

	return ch;
}
