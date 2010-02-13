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
  char ctype;
  phase_type ptype;
  phase *phase;
  attr_type atype;
}

%token <stype> KEYWORD RESERVED NULLABLE
%token <stype> BEFORE AFTER
%token <stype> STRING
%token <ctype> ENDL
%token <stype> BLOCK_SET

%left ':'

%type <stype> line;
%type <stype> attr;
%type <ptype> phase_decl;
%type <phase> phase hook;
%type <atype> attr_decl;
%type <stype> block;

%%

// grammar
program:
  program decl              {};
  | decl
  ;
    
decl:
  phase                 {debug(2, "Found phase in program: %p\n", $1);}
  | hook                {debug(2, "Found a hook in the program\n");}
  | attr                {debug(2, "Found new attribute in program\n");}
  | '\n'                /* NULL */
  ;

phase:
  phase_decl line           {
    // Set the phase and attach it to the config object
    phase *p = find_or_create_phase(config, $1);
    p->command = (char *)malloc(sizeof(char *) * strlen($2));
    p->command = strdup($2);
    debug(3, "Found a phase: [%s %s]\n", phase_type_to_string(p->type), p->command); 
    add_phase(config, p);
  }
  | phase_decl block        {
    // I think these two can be combined... I hate code duplication
    phase *p = find_or_create_phase(config, $1);
    p->command = (char *)malloc(sizeof(char *) * strlen($2));
    p->command = strdup($2);
    debug(3, "Found a phase: [%s %s]\n", phase_type_to_string(p->type), p->command); 
    add_phase(config, p);
  }
  | phase_decl NULLABLE         {
    debug(3, "Found a nullable phase_decl: %s\n", phase_type_to_string($1));
    phase *p = find_or_create_phase(config, $1);
    add_phase(config, p);
  }
  ;

phase_decl:
  KEYWORD ':'             {$$ = str_to_phase_type($1);}
  | KEYWORD               {$$ = str_to_phase_type($1);}
  ;

// Hooks
// TODO: Make sure blocks work here
hook:
  BEFORE ':' line          {
    debug(3, "Found a hook phrase: %s (%s)\n", $3, $1);
    phase_type t = str_to_phase_type($1);
    // Do some error checking on the type. please
    phase *p = find_or_create_phase(config, t);
    p->before = (char *)malloc(sizeof(char *) * strlen($3));
    p->before = strdup($3);
    add_phase(config, p);
  }
  | AFTER ':' line          {
    debug(3, "Found a hook phrase: %s (%s)\n", $3, $1);
    phase_type t = str_to_phase_type($1);
    // Do some error checking on the type. please
    phase *p = find_or_create_phase(config, t);
    p->after = (char *)malloc(sizeof(char *) * strlen($3));
    p->after = strdup($3);
    add_phase(config, p);
  }
  ;

// Attributes
attr:
  attr_decl line            {
    debug(3, "Found an attribute: [%s %s]\n", attribute_type_to_string($1), $2);
    add_attribute(config, $1, $2);
  }
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
