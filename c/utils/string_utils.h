#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <ctype.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAXLINE
#define MAXLINE 1024
#endif
 
/* Count the number of arguments. */
 
int count_args (const char * input);
int copy_args (const char * input, int argc, char ** argv);
int argify(const char *line, char ***argv_ptr);
char* commandify(int argc, const char** argv);
char* chomp(char *string);

#ifdef __cplusplus
}
#endif


#endif
