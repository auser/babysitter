%{
#include <stdio.h>
#include <string.h>

#include "honeycomb_config.h"
#include "hc_support.h"
#include "print_utils.h"

extern int yylineno;
extern char *current_parsed_file;
%}

%union {
  int i; 
  char* stype;
  char ctype;
  phase_type ptype;
  phase_t *phase;
  attr_type atype;
}

%token <stype> KEYWORD NULLABLE
%token <stype> BEFORE AFTER
%token <stype> STRING
%token <ctype> ENDL
%token <stype> BLOCK_SET
%token <atype> RESERVED

%left ':'

%type <stype> line;
%type <stype> attr;
%type <ptype> phase_decl;
%type <phase> phase hook;
%type <stype> block;

%%

// grammar
program:
  program decl              {};
  | decl                    {}
  ;
    
decl:
  phase                 {debug(DEBUG_LEVEL, 2, "Found phase in program: %p at %d\n", $1, yylineno);}
  | hook                {debug(DEBUG_LEVEL, 2, "Found a hook in the program at %d\n", yylineno);}
  | attr                {debug(DEBUG_LEVEL, 2, "Found new attribute in program at: %d\n", yylineno);}
  | '\n'                {++yylineno;}/* NULL */
  ;

phase:
  phase_decl line           {
    debug(DEBUG_LEVEL, 2, "Found phase: %s\n", $1);
    // Set the phase and attach it to the config object
    phase_t *p = find_or_create_phase((honeycomb_config*)config, $1);
    add_phase_attribute(p, T_COMMAND, $2);
    add_phase((honeycomb_config*)config, p);
    free($2);
  }
  | phase_decl block        {
    // I think these two can be combined... I hate code duplication
    phase_t *p = find_or_create_phase((honeycomb_config*)config, $1);
    add_phase_attribute(p, T_COMMAND, $2);
    add_phase((honeycomb_config*)config, p);
    free($2);
  }
  | phase_decl NULLABLE         {
    debug(DEBUG_LEVEL, 3, "Found a nullable phase_decl: %s\n", phase_type_to_string($1));
    phase_t *p = find_or_create_phase((honeycomb_config*)config, $1);
    add_phase((honeycomb_config*)config, p);
  }
  ;

phase_decl:
  KEYWORD ':'             {$$ = str_to_phase_type($1); } // free($1);
  | KEYWORD               {$$ = str_to_phase_type($1); } // free($1);
  ;

// Hooks
// TODO: Make sure blocks work here
hook:
  BEFORE ':' line          {
    debug(DEBUG_LEVEL, 3, "Found a hook phrase: %s (%s)\n", $3, $1);
    phase_type t = str_to_phase_type($1);
    // Do some error checking on the type. please
    phase_t *p = find_or_create_phase((honeycomb_config*)config, t);
    add_phase_attribute(p, T_BEFORE, $3);
    add_phase((honeycomb_config*)config, p);
    free($1); free($3);
  }
  | BEFORE ':' block        {
    debug(DEBUG_LEVEL, 3, "Found a hook phrase: %s (%s)\n", $3, $1);
    phase_type t = str_to_phase_type($1);
    // Do some error checking on the type. please
    phase_t *p = find_or_create_phase((honeycomb_config*)config, t);
    add_phase_attribute(p, T_BEFORE, $3);
    add_phase((honeycomb_config*)config, p);
    free($1); free($3);
  }
  | AFTER ':' line          {
    debug(DEBUG_LEVEL, 3, "Found a hook phrase: %s (%s)\n", $3, $1);
    phase_type t = str_to_phase_type($1);
    // Do some error checking on the type. please
    phase_t *p = find_or_create_phase((honeycomb_config*)config, t);
    add_phase_attribute(p, T_AFTER, $3);
    add_phase((honeycomb_config*)config, p);
    free($1); free($3);
  }
  | AFTER ':' block         {
    debug(DEBUG_LEVEL, 3, "Found a hook phrase: %s (%s)\n", $3, $1);
    phase_type t = str_to_phase_type($1);
    // Do some error checking on the type. please
    phase_t *p = find_or_create_phase((honeycomb_config*)config, t);
    add_phase_attribute(p, T_AFTER, $3);
    add_phase((honeycomb_config*)config, p);
    free($1); free($3);
  }
  ;

// Attributes
attr:
  RESERVED ':' line              {
    debug(DEBUG_LEVEL, 4, "Found reserved: %d : %s\n", $1, $3);
    add_config_attribute((honeycomb_config*)config, $1, $3);
    free($3);
  }
  | RESERVED ':' NULLABLE     {
    debug(DEBUG_LEVEL, 4, "Found empty attribute\n");
  }
  ;
  
// Blocks
block:
  BLOCK_SET                   {debug(DEBUG_LEVEL, 3, "Found a block\n");$$ = $1;}
  ;

// Line terminated by '\n'
line:
  line STRING                     {debug(DEBUG_LEVEL, 3, "Found line: '%s'\n", $1);strcpy($$,$1);}
  | STRING                        {debug(DEBUG_LEVEL, 3, "Found line: '%s'\n", $1);strcpy($$,$1);}
  ;

%%

int yyerror(const char *str)
{
  fprintf(stderr, "Parsing error %s [%i]: %s\n", current_parsed_file, yylineno, str);
  return 0;
}
