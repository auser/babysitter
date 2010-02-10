%{
#include <stdio.h>
#include <string.h>
#include "honeycomb_config.h"
#include "c_ext.h"

extern int yylineno;
%}

%union {
  int i; 
  char* stype;
  char ctype;
}

%token <stype> BUNDLE START STOP MOUNT UNMOUNT CLEANUP PERIOD 
%token <stype> BEFORE AFTER
%token <stype> STRING FILENAME
%token <ctype> WHITESPACE

%%

// grammar

phase:
  phase_decl line             {debug(3, "Found a phase: %s\n", "2");}
  | phase_decl FILENAME       {debug(4, "Found phase filename: %s\n", $2);}
  | phase_decl                {debug(4, "Found empty phase\n");}
  ;

phase_decl:
  BUNDLE ':'               {debug(3, "Found a phase_decl: %s\n", $1);}
  | START ':'              {debug(3, "Found start decl: %s\n", $1);}
  ;
  
line:
  line STRING                 {debug(4, "found a line: %s\n", $2);}
  | STRING                    {debug(4, "Found a string (in grammar): %s\n", $1);}
  ;

%%

int yyerror(const char *str)
{
  fprintf(stderr, "Parsing error [%i]: %s\n", yylineno, str);
  return 0;
}
