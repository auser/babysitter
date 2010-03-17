#include "string_utils.h"

#define SKIP(p) while (*p && isspace (*p)) p++
#define WANT(p) *p && !isspace (*p)

/* Count the number of arguments. */
int count_args (const char * input)
{
  const char * p;
  int argc = 0;
  p = input;
  while (*p) {
    SKIP (p);
    if (WANT (p)) {
      argc++;
      while (WANT (p)) p++;
      }
  }
  return argc;
}
 
/* Copy each non-whitespace argument into its own allocated space. */
 
int copy_args (const char * input, int argc, char ** argv)
{
  int i = 0;
  const char *p;
  p = input;
  while (*p) {
    SKIP (p);
    if (WANT (p)) {
      const char* end = p;
      char* copy;
      while (WANT (end)) end++;
      copy = argv[i] = (char *)malloc (end - p + 1);
      if (! argv[i]) return -1;
      while (WANT (p)) *copy++ = *p++;
      *copy = 0;
      i++;
    }
  }
  if (i != argc) return -1;
  return 0;
}
 
int argify(const char *line, char ***argv_ptr)
{
  int argc;
  char ** argv;

  argc = count_args (line);
  if (argc == 0)
      return -1;
  argv = (char **)malloc (sizeof (char *) * argc);
  if (! argv) return -1;
  if (copy_args (line, argc, argv) < 0) return -1;
  *argv_ptr = argv;
  
  return argc;
}

#undef SKIP
#undef WANT

char* commandify(int argc, const char** argv)
{
  int curr_pos = 0, i = 0, j = 0, total, len;
  char *cmd;
  
  for (i = 0; i < argc; i++) total += strlen(argv[i]) + 1; // plus a space
  
  cmd = (char*)malloc( sizeof(char *) * total + 1 );
  
  while(j < argc) {
    len = strlen(argv[j]) + 1;
    for(i=0; i < len; i++) {
      if (i == len-1)
        cmd[curr_pos++] = ' ';
      else
        cmd[curr_pos++] = argv[j][i];
    }
    j++; 
  }
  cmd[curr_pos-1] = 0; // End it with a null-terminated character
  return cmd;
}

#undef SKIP
#undef WANT

char* chomp(char *string)
{
  char *s, *t, *res;
  int len = strlen(string);
  res = (char*)malloc(sizeof(char*) * len);
  
  res = strdup(string);
  for (s = res; isspace (*s); s++) ;
  
  if (*s == 0) return (s);
  
  t = s + strlen (s) - 1;
  while (t > s && isspace(*t)) t--;
  *++t = '\0';
  return s;
}
