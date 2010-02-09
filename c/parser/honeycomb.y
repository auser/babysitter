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

%token <str> PHASE HOOK
%token <str> WORD LINE
%token <char> SEMICOLON QUOTE OBRACE EBRACE WHITESPACE COLON PERIOD

%%

// grammar

other:
  PERIOD {printf(">SD>Fd\n");}
  | LINE {printf("line: %s\n", $1);}
  | PHASE {printf("Phase: %s\n", $1);}
  ;

%%

int yyerror(const char *str)
{
  fprintf(stderr, "Parsing error [%i]: %s\n", yylineno, str);
  return 0;
}
