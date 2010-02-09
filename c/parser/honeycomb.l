%{
#include <stdio.h>
#include <string.h>
#include "y.tab.h"

char buf[100]; 
char *s;
%}

PHASE       (bundle|start|stop|mount|unmount|cleanup)
HOOK        [before|after]

WORD        [a-zA-Z][a-zA-Z0-9]*
LINE		    ^(.*)\n
SEMICOLON   ;
COLON       :
PERIOD      \.
COMMENT     #
WHITESPACE  [ \t]+

%%
{PHASE}                 {
                          printf("Phase: %s\n", yytext);
                          yylval.str = yytext;
                          return PHASE;
                        }
{HOOK}                  {
                          printf("Hook: %s\n", yytext);
                          yylval.str = yytext;
                          return HOOK;
                        }
{WORD}                  {
                          printf("WORD: %s\n", yytext);
                          return WORD;
                        }
{COLON}                 {
                          return COLON;
                        }
{COMMENT}               /* ignore comments */;
{WHITESPACE}            /* ignore whitespace */;
{PERIOD}                {
                          printf("PERIOD\n ");
                          return PERIOD;
                        }
{LINE}                  {
                          printf("LINE: %s\n", yytext);
                          ++yylineno;
                          return LINE;
                        }
.                       /* ignore everything else */;

%%
