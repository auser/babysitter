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
  char** btype;
  char ctype;
  phase_type ptype;
}

%token <stype> KEYWORD
%token <stype> BEFORE AFTER
%token <stype> STRING DIRECTORIES EXECUTABLES ENV STDOUT STDIN
%token <ctype> ENDL
%token <btype> BLOCK_SET

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
  KEYWORD ':'               {$$ = T_BUNDLE;}
  ;  

line:
  line STRING                 {strcpy($$,strcat($$,$2));}
  | line ENDL                 {debug(4, "Found the end of the line\n");}
  | STRING                    {strcpy($$,$1);}
  ;

%%

int yyerror(const char *str)
{
  fprintf(stderr, "Parsing error [%i]: %s\n", yylineno, str);
  return 0;
}
