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
  phase_type ptype;
}

%token <stype> BUNDLE START STOP MOUNT UNMOUNT CLEANUP PERIOD 
%token <stype> BEFORE AFTER
%token <stype> STRING
%token <ctype> ENDL

%left ':'

%type<stype> line;
%type <ptype> phase_decl;

%%

// grammar

phase:
  | phase_decl line           {debug(3, "Found a phase: [%s %s]\n", ptype_to_string($1), $2);}
  | phase_decl ENDL           {debug(4, "Found empty phase\n");}
  ;

phase_decl:
  BUNDLE ':'               {debug(3, "Found a phase_decl: %d\n", $1); $$ = T_BUNDLE;}
  | START ':'              {debug(3, "Found start decl: %d\n", $1);}
  ;  

line:
  line STRING                 {strcpy($$,strcat($$,$2));}
  | STRING                    {strcpy($$,$1);}
  ;

%%

int yyerror(const char *str)
{
  fprintf(stderr, "Parsing error [%i]: %s\n", yylineno, str);
  return 0;
}
