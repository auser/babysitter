%{
#include <stdio.h>
#include <string.h>
#include "honeycomb_config.h"
#include "c_ext.h"

extern int yylineno;
%}

%union {
  int i; 
  char* str;
}

%token PHASE_CMD HOOK_CMD

%%

// grammar

phase:
  HOOK_CMD
  ;

%%

int yyerror(const char *str)
{
  fprintf(stderr, "Parsing error [%i]: %s\n", yylineno, str);
  return 0;
}
