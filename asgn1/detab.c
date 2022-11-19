#include <stdio.h>

int main(void) {
	int c;
	int count = 0;

	while((c = getchar()) != EOF){
		if(c == '\t') {
			/* Fancy mod arithmetic 
 * 			to figure out how many spaces we need. */
			int temp = (8 - (count % 8));  
			
			int i;
			for(i = 0; i < temp; i++) {
				putchar(' ');
			}

			/* Remember, we just added this many spaces,
 *  			 so count should follow suit. */
			count += temp;
			/* Skip over outputting this char. */ 
			continue;  
		}
		else if(c == '\n') {
			/* New line, new count. */
			count = 0;  
		}
		else if(c == '\b') {
			/* Decrement count while avoiding
 * 			 backspacing through margin. */
			if(count != 0) {
				count--;  
			}
		}
		else if(c == '\r') {
			/* Returning to beginning of line, so count is zero. */
			count = 0;  
		}
		else {
			/* Otherwise, just bump the count
 * 			 with the character we're about to add. */
			count++;  
		}	
		/* Output the given character. */
		putchar(c);  
	}

	return 0;
}
