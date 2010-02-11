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
  attr_type atype;
}

%token <stype> KEYWORD RESERVED
%token <stype> BEFORE AFTER
%token <stype> STRING
%token <ctype> ENDL
%token <btype> BLOCK_SET

%left ':'

%type<stype> line;
%type <stype> attr;
%type <ptype> phase_decl;
%type <atype> attr_decl;

%%

// grammar
program:
  phase
  | attr
  ;

phase:
  phase_decl line           {debug(3, "Found a phase: [%s %s]\n", ptype_to_string($1), $2);}
  | phase_decl ENDL           {debug(4, "Found empty phase\n");}
  ;

attr:
  attr_decl line            {debug(3, "Found an attribute: [%d %s]\n", $1, $2);}
  | attr_decl ENDL          {debug(4, "Found empty attribute\n");}
  ;
    
phase_decl:
  KEYWORD ':'                 {
                                if (strcmp($1,"bundle") == 0) $$ = T_BUNDLE;
                                else if (strcmp($1,"start") == 0) $$ = T_START;
                                else if (strcmp($1,"stop") == 0) $$ = T_STOP;
                                else if (strcmp($1,"mount") == 0) $$ = T_MOUNT;
                                else if (strcmp($1,"unmount") == 0) $$ = T_UNMOUNT;
                                else if (strcmp($1,"cleanup") == 0) $$ = T_CLEANUP;
                                else exit(-1);
                              }
  ;

// Attributes
attr_decl:
  RESERVED ':'                {
                                if (strcmp($1,"executables") == 0) $$ = T_EXECUTABLES;
                                else if (strcmp($1,"directories") == 0) $$ = T_DIRECTORIES;
                                else if (strcmp($1,"env") == 0) $$ = T_ENV;
                                else if (strcmp($1,"stdout") == 0) $$ = T_STDOUT;
                                else if (strcmp($1,"stdin") == 0) $$ = T_STDIN;
                                else if (strcmp($1,"root_dir") == 0) $$ = T_ROOT_DIR;
                                else exit(-1);
                              }
  ;

// Line
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
