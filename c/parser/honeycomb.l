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
PERIOD      \.
COLON       :
COMMENT     #
WHITESPACE  [ \t\r]

ALPHA			[A-Za-z]
DIGIT			[0-9]
ALPHANUM	[A-Za-z0-9]

LINE      {ALPHA}({WHITESPACE}*{ALPHANUM})
FILENAME  [a-zA-Z0-9\/.-]+

ANYTHING  (.*)
NEWLINE   '\n'
EMPTYLINE [ '\t''\n']+

%x PHASE

%%
{WHITESPACE}			    ;		/* ignore white space */
  /* Keywords */
bundle              yylval.str = strdup("bundle"); return BUNDLE;
start               yylval.str = strdup("start"); return START;
stop                return STOP;
mount               return MOUNT;
unmount             return UNMOUNT;
cleanup             return CLEANUP;
before              return BEFORE;
after               return AFTER;

:                   return DECL_SEP;
{LINE}             {
                      debug(3, "%4d\tFound Line: %s\n", yylineno, yytext);
                      yylval.str = strdup(yytext);
                      return LINE;
                    }

{FILENAME}          yylval.str = strdup(yytext); return FILENAME;
'.'                 return PERIOD;
\"                  return QUOTE;
\{                  return OBRACE;
\}                  return EBRACE;

{COMMENT}{ANYTHING}   {debug(4, "%4d\tIgnoring comment: %s\n", yylineno, yytext);} /* ignore comments */
{EMPTYLINE}           /* ignore */ ++yylineno;

%%
