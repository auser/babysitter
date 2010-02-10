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
WHITESPACE  [ \t\r]
ALPHANUM	[A-Za-z0-9]
FILENAME  [a-zA-Z0-9\/._-]+

ANYTHING  (.*)
EMPTYLINE [ \t]+\n

%x QUOTEDSTR

%%

{WHITESPACE}			    ;		/* ignore white space */
  /* Keywords */
^bundle              yylval.stype = strdup("bundle"); return BUNDLE;
^start               yylval.stype = strdup("start"); return START;
stop                return STOP;
mount               return MOUNT;
unmount             return UNMOUNT;
cleanup             return CLEANUP;
before              return BEFORE;
after               return AFTER;

[\(\)\{\}\:;=,\.\\n]  { debug(4, "Found char: %c\n", yytext[0]);return yytext[0]; }
[a-zA-Z0-9]+            {
                          debug(4, "Found string: %s\n", yytext);
                        	yylval.stype = strdup(yytext);
                        	return STRING;
                        }

{FILENAME}          {debug(4, "Found filename: %s\n", yytext); yylval.stype = strdup(yytext); return FILENAME;}

  /* Strings */
\"                  {
  debug(4, "Saw quote, so moving into string state\n");
  memset(buf, 0, 1024);
  s = buf;
  BEGIN QUOTEDSTR;
}
<QUOTEDSTR>\\n         {*s++ = '\n';}
<QUOTEDSTR>\\t         {*s++ = '\t';}
<QUOTEDSTR>\\\"        {*s++ = '\"';}
<QUOTEDSTR>\"          {
  *s = 0;
  BEGIN 0;
  printf("String: %s\n", buf);
  yylval.stype = strdup(buf);
  return STRING;
}
<QUOTEDSTR>\n          {debug(3, "Not sure... \n");}
<QUOTEDSTR>.           {*s++ = *yytext;}

{COMMENT}{ANYTHING} {debug(4, "%4d\tIgnoring comment: %s", yylineno, yytext);} /* ignore comments */
{EMPTYLINE}         debug(4, "%4d\tEmpty line\n", yylineno);++yylineno;  /* ignore */ 
.                   debug(4, "%4d\tIgnoring: %s", yytext);

%%
