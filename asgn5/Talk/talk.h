#ifndef TALKH
#define TALKH

#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <curses.h>

/* ncurses windowing helpers.  Be sure to link with -lncurses */
void start_windowing();         /* turn on windowing  */
void stop_windowing();          /* turn off windowing */

void update_input_buffer();     /* Pull in any pending data (nonblocking)  */
int  has_whole_line();          /* Does row buffer hold a line (non-blocking) */
int  has_hit_eof();             /* Are we done? */
int  read_from_input(char *buf, size_t len); /* may block */
int  write_to_output(const char *buf, size_t len);
int  fprint_to_output(const char *fmt, ...);  /* mostly useful for debugging */

/* dial the verbosity of error reporting */
int set_verbosity(int level);

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif


#endif
