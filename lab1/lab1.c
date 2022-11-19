#include <stdio.h>
int main(void) {
	char input[33];

	printf("What do you want me to echo? ");
	scanf("%32s", input);
	printf("\nEcho! %s\n", input);

	return 0;
}
