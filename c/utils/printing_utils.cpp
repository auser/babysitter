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