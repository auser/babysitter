%{
#include <stdio.h>
#include <string.h>
#include "honeycomb_config.h"
#include "c_ext.h"
#include "y.tab.h"

char buf[1024]; 
char *s;
%}

/* Reserved words */
BUNDLE      bundle
START       start
STOP        stop
MOUNT       mount
UNMOUNT     unmount
CLEANUP     cleanup
BEFORE      before
AFTER       after

/* Others */
PERIOD      \.
COLON       :
COMMENT     #
WHITESPACE  [ \t\t]

ALPHA			[A-Za-z]
DIGIT			[0-9]
ALPHANUM	[A-Za-z0-9]

LINE        [a-zA-Z0-9 ]+
EMPTYLINE   [\t\n]+

%x PHASE

%%
WHITESPACE			      break;		/* ignore white space */
  /* keywords */
{BUNDLE}              return BUNDLE;
{START}               return START;
{STOP}                return STOP;
{MOUNT}               return MOUNT;
{UNMOUNT}             return UNMOUNT;
{CLEANUP}             return CLEANUP;
{PERIOD}              return PERIOD;
{BEFORE}              return BEFORE;
{AFTER}               return AFTER;

{ALPHA}{ALPHANUM}*    return LINE;

{COLON}               return DECL_SEP;

{EMPTYLINE}           /* ignore */ ++yylineno;
{COMMENT}{LINE}       debug(3, "%4d\tIgnoring comment: %s\n", yylineno, yytext); /* ignore comments */
(.*)                  debug(3, "%4d\t%s\n", yylineno, yytext);/* Ignore everything else */;

%%
