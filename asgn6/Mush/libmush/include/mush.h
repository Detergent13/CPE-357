#ifndef MUSHH
#define MUSHH
#include <stdio.h>

typedef struct clstage *clstage;

struct clstage {
  char *inname;                 /* input filename (or NULL for stdin) */
  char *outname;                /* output filename (NULL for stdout)  */
  int   argc;                   /* argc for the stage                 */
  char **argv;                  /* Array for argv (NULL Terminated)   */

  clstage next;                 /* link pointer for listing in the parser */
};

typedef struct pipeline {
  char           *cline;              /* the original command line  */
  int            length;              /* length of the pipeline     */
  struct clstage *stage;              /* descriptors for the stages */
} *pipeline;

void     print_pipeline(FILE *where, pipeline cl);
void     free_pipeline(pipeline cl);
pipeline crack_pipeline(char *line);

extern int lineno;                    /* defined in the parser */
extern int clerror;                   /* defined in the parser (what err) */

#define E_NONE       0
#define E_NULL       ( E_NONE   + 1 )
#define E_EMPTY      ( E_NULL   + 1 )
#define E_BADIN      ( E_EMPTY  + 1 )
#define E_BADOUT     ( E_BADIN  + 1 )
#define E_BADSTR     ( E_BADOUT + 1 )
#define E_PARSE      ( E_BADSTR + 1 )

char *readLongString(FILE *infile);
int yylex_destroy(void);  /* tear down flex's data structures */
#endif

