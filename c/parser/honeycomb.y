%{
#include <stdio.h>
#include <string.h>
#include "honeycomb_config.h"
#include "hc_support.h"

extern int yylineno;
%}

%union {
  int i; 
  char* stype;
  char** btype;
  char ctype;
  phase_type ptype;
  phase phase;
  attr_type atype;
}

%token <stype> KEYWORD RESERVED NULLABLE
%token <stype> BEFORE AFTER
%token <stype> STRING
%token <ctype> ENDL
%token <btype> BLOCK_SET

%left ':'

%type <stype> line hook_decl;
%type <stype> attr;
%type <ptype> phase_decl;
%type <phase> phase;
%type <atype> attr_decl;
%type <btype> block;

%%

// grammar
program:
  program decl              {};
  | decl
  ;
    
decl:
  phase                 {
    debug(1, "Config: %p\n", ((honeycomb_config *) config)->app_type);
    debug(1, "Found phase in program: %p\n", $1);
  }
  | hook                {debug(1, "Found a hook in the program\n");}
  | attr                {debug(1, "Found new attribute in program\n");}
  | '\n'                /* NULL */
  ;

phase:
  phase_decl line           {
    debug(3, "Found a phase: [%s %s]\n", phase_type_to_string($1), $2); 
    phase *p = new_phase($1);
    add_phase(config, p);
  }
  | phase_decl block        {debug(3, "Found a block phrase: %s\n", $2); }
  | phase_decl NULLABLE         {debug(3, "Found a nullable phase_decl: %s\n", phase_type_to_string($1));}
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

// Hooks
hook:
  hook_decl line          {debug(3, "Found a hook phrase: %s\n", $2); }
  | hook_decl block         {debug(3, "Found a hook block: %s\n", $2); }
  ;
hook_decl:
  BEFORE ':'                  {debug(2, "Found hook: %s\n", $1); $$ = $1;}
  | AFTER ':'                 {debug(2, "Found after hook: %s\n", $1), $$ = $1;}
  ;

// Attributes
attr:
  attr_decl line            {debug(3, "Found an attribute: [%s %s]\n", attribute_type_to_string($1), $2);}
  | attr_decl               {debug(4, "Found empty attribute\n");}
  ;
  
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

// Blocks
block:
  BLOCK_SET                   {debug(3, "Found a block\n");$$ = $1;}
  ;

// Line terminated by '\n'
line:
  line STRING                      {debug(3, "Found string: '%s'\n", $1);strcpy($$,$1);}
  | STRING
  ;

%%

int yyerror(const char *str)
{
  fprintf(stderr, "Parsing error [%i]: %s\n", yylineno, str);
  return 0;
}
