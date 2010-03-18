#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "printing_utils.h"

void fperror(const char *s,...)
{
  char buf[1024];
  va_list p;

	va_start(p, s);
	/* Guard against "<error message>: Success" */
	vsprintf(buf, s, p);
	va_end(p);
}

void fperror_and_die(int exit_code, const char *s, ...)
{
  va_list p;
	
  fperror(s, p);
	exit(exit_code);
}

// Debug
int debug(const long int cur_level, int level, char *fmt, ...)
{
  int r;
  va_list ap;
  if (cur_level < level) return 0;
	va_start(ap, fmt);
  fprintf(stderr, "[debug %d] ", level);
	r = vfprintf(stderr, fmt, ap);
	va_end(ap);
	return r;
}
