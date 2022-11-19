/*
 * cpl:  A shell pipeline cracker
 *
 * Author: Dr. Phillip Nico
 *         Department of Computer Science
 *         California Polytechnic State University
 *         One Grand Avenue.
 *         San Luis Obispo, CA  93407  USA
 *
 * Email:  pnico@calpoly.edu
 *
 * Revision History:
 *         $Log: main.c,v $
 *         Revision 1.2  2020-12-17 15:24:57-08  pnico
 *         adjusted for the new behavior of free_pipelie() not freeing cline
 *
 *         Revision 1.1  2020-12-17 09:06:06-08  pnico
 *         Initial revision
 *
 *
 */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <mush.h>

#ifndef TRUE
#define TRUE (1==1)
#endif

#ifndef FALSE
#define FALSE (0==1)
#endif

#define PROMPT "line: "

static void printusage(char *name);
static void prompt(char *prompt);

int main(int argc, char *argv[]){
  char *line;
  pipeline pl;
  int run;
  char *promptstr;

  /* check for the right number of arguments */
  if ( argc > 1 ) {
    printusage(argv[0]);
    exit(-1);
  }

  /* set prompt */
  promptstr = PROMPT;

  setenv("PARSELINE","whatever",1);

  run = TRUE;
  prompt(promptstr);
  while ( run ) {
    if ( NULL == (line = readLongString(stdin)) ) {
      if ( feof(stdin) )
        run = FALSE;
    } else {
      /* We got a line, send it off to the pipeline cracker and
       * launch it
       */
      pl = crack_pipeline(line);

      /*
       * Show that it worked.  This is where you're going to do
       * something radically different: rather than printing the
       * pipeline, you're going to execute it.
       */
      if ( pl != NULL ) {
        print_pipeline(stdout,pl); /* print it. */
        free_pipeline(pl);
      } else {
        if ( clerror == E_EMPTY )
          fprintf(stderr,"Invalid null command, line %d.\n", lineno);
      }

      lineno++;  /* readLongString trims newlines, so increment it manually */
      free(line);
    }
    if (run )                 /* assuming we still want to run */
      prompt(promptstr);
  }

  return 0;
}

static void prompt(char *pr) {
  /* If this is an interactive shell, flush the output streams and
   * print a prompt
   */

  if ( isatty(STDIN_FILENO) && isatty(STDOUT_FILENO) ) {
    printf("%s", pr);
    fflush(stdout);
  }
}

static void printusage(char *name){
  fprintf(stderr,"usage: %s\n",name);
}
