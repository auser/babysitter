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
  char c;
}

%token <str> BUNDLE START STOP MOUNT UNMOUNT CLEANUP PERIOD 
%token <str> BEFORE AFTER
%token <str> LINE FILENAME
%token <c> DECL_SEP NEWLINE WHITESPACE QUOTE OBRACE EBRACE

%%

// grammar

phase:
  phase_decl LINE NEWLINE             {debug(3, "Found a phase: %s\n", $2);}
  | phase_decl NEWLINE          {debug(4, "Found empty phase\n");}
  ;

phase_decl:
  BUNDLE DECL_SEP               {debug(3, "Found a phase_decl: %s\n", $1);}
  | START DECL_SEP              {debug(3, "Found start decl: %s\n", $1);}
  ;

%%

int yyerror(const char *str)
{
  fprintf(stderr, "Parsing error [%i]: %s\n", yylineno, str);
  return 0;
}
