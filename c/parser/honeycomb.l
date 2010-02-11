%{
#include <stdio.h>
#include <string.h>
#include "honeycomb_config.h"
#include "c_ext.h"
#include "y.tab.h"

#define BUF_SIZE 1024

char buf[BUF_SIZE]; 
char *s;
%}

/* Others */
COMMENT     #
ENDL        \n
WHITESPACE  [ \t\r]
STRING  [a-zA-Z0-9\/._-]+

ANYTHING  (.*)
EMPTYLINE [ \t\r]*\n

%x QUOTEDSTR
%x BLOCK

%%

{WHITESPACE}			    ;		/* ignore white space */

  /* Keywords */
bundle              debug(4, "next phase: bundle\n"); yylval.ptype = T_BUNDLE; return BUNDLE;
start               debug(4, "next phase: start\n"); yylval.ptype = T_START; return START;
stop                debug(4, "next phase: stop\n"); yylval.ptype = T_STOP; return STOP;
mount               debug(4, "next phase: mount\n"); return MOUNT;
unmount             debug(4, "next phase: unmount\n"); return UNMOUNT;
cleanup             debug(4, "next phase: cleanup\n"); return CLEANUP;
before              debug(4, "next hook: before\n"); return BEFORE;
after               debug(4, "next hook: after\n"); return AFTER;

[\(\)\{\}\:;=,\.]   { debug(4, "Found char: %c\n", yytext[0]);return yytext[0]; }

  /* Strings */
{STRING}({WHITESPACE}?({STRING}))*  {
                    	yylval.stype = strdup(yytext);
                    	debug(4, "returning STRING: '%s'\n", yytext);
                    	return STRING;
                    }

\"                  {
  memset(buf, 0, BUF_SIZE);
  s = buf;
  BEGIN QUOTEDSTR;
}
<QUOTEDSTR>\\n         {*s++ = '\n';}
<QUOTEDSTR>\\t         {*s++ = '\t';}
<QUOTEDSTR>\\\"        {*s++ = '\"';}
<QUOTEDSTR>\"          {
  *s = 0;
  BEGIN INITIAL; // get outta here
  yylval.stype = strdup(buf);
  debug(4, "returning STRING: '%s'\n", yytext);
  return STRING;
}
<QUOTEDSTR>\n           {fprintf(stderr, "Error: %4d\tInvalid string\n", yylineno); BEGIN 0;}
<QUOTEDSTR>.            {*s++ = *yytext;}

  /* blocks */

  /* everything else */
{COMMENT}{ANYTHING}\n   {debug(4, "%4d\tIgnoring comment: %s", ++yylineno, yytext);} /* ignore comments */
{EMPTYLINE}             debug(4, "%4d\tEmpty line\n", ++yylineno);  /* ignore */ 

%%
