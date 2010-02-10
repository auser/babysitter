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

%token <str> BUNDLE START STOP MOUNT UNMOUNT CLEANUP PERIOD 
%token <str> BEFORE AFTER
%token <str> LINE
%token <char> DECL_SEP COLON

%%

// grammar

phase:
  BUNDLE COLON LINE
  ;

%%

int yyerror(const char *str)
{
  fprintf(stderr, "Parsing error [%i]: %s\n", yylineno, str);
  return 0;
}
