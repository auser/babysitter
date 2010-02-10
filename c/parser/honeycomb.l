%{
#include <stdio.h>
#include <string.h>
#include "honeycomb_config.h"
#include "c_ext.h"
#include "y.tab.h"

char buf[1024]; 
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

%%

{WHITESPACE}			    ;		/* ignore white space */
  /* Keywords */
^bundle              yylval.ptype = T_BUNDLE; return BUNDLE;
^start               yylval.ptype = T_START; return START;
^stop                yylval.ptype = T_STOP; return STOP;
^mount               return MOUNT;
^unmount             return UNMOUNT;
^cleanup             return CLEANUP;
before              return BEFORE;
after               return AFTER;

[\(\)\{\}\:;=,\.]   { debug(4, "Found char: %c\n", yytext[0]);return yytext[0]; }
  /* Strings */
{STRING}({WHITESPACE}?({STRING}))*  {
                    	yylval.stype = strdup(yytext);
                    	debug(4, "returning STRING: '%s'\n", yytext);
                    	return STRING;
                    }

\"                  {
  memset(buf, 0, 1024);
  s = buf;
  BEGIN QUOTEDSTR;
}
<QUOTEDSTR>\\n         {*s++ = '\n';}
<QUOTEDSTR>\\t         {*s++ = '\t';}
<QUOTEDSTR>\\\"        {*s++ = '\"';}
<QUOTEDSTR>\"          {
  *s = 0;
  BEGIN 0; // get outta here
  yylval.stype = strdup(buf);
  return STRING;
}
<QUOTEDSTR>\n           {fprintf(stderr, "Error: %4d\tInvalid string\n", yylineno); BEGIN 0;}
<QUOTEDSTR>.            {*s++ = *yytext;}

{COMMENT}{ANYTHING}\n   {debug(4, "%4d\tIgnoring comment: %s", ++yylineno, yytext);} /* ignore comments */
{EMPTYLINE}             debug(4, "%4d\tEmpty line\n", ++yylineno);  /* ignore */ 
{ENDL}$                 debug(4, "%4d\tNewline\n", yylineno);return ENDL;
.                       debug(4, "%4d\tIgnoring: %s", yytext);

%%
