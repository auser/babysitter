/* Reserved words */
BUNDLE      bundle
START       start
STOP        stop
MOUNT       mount
UNMOUNT     unmount
CLEANUP     cleanup
BEFORE      before
AFTER       after



/* Rules */
PHASE_CMD   ({BUNDLE}|{START}|{STOP}|{MOUNT}|{UNMOUNT}|{CLEANUP})
HOOK_CMD    (\.)+({BEFORE}|{AFTER})

/* Others */
COLON       :
COMMENT     #
WHITESPACE  [ \t]+

LINE        (.*)
EMPTYLINE   [\t\n]+

%{
#include <stdio.h>
#include <string.h>
#include "y.tab.h"

char buf[1024]; 
char *s;
%}
%x PHASE

%%
^{PHASE_CMD}          {
                        printf("Phase: %s\n", yytext);
                        BEGIN PHASE;
                        s = buf;
                      };
<PHASE>{COLON}{LINE}  {}
<PHASE>{HOOK_CMD}     {
                        printf("Hook command: %s\n", yytext);
                      }

{HOOK_CMD}    {
                printf("Hook: %s\n", yytext);
                //return HOOK_CMD;
              }

{EMPTYLINE}           /* ignore */ ++yylineno;
{COMMENT}{LINE}       /* ignore comments */;
{COMMENT}{LINE}       /* ignore comments */;
(.*)          printf("%4d\t%s\n", yylineno, yytext);/* Ignore everything else */;

%%
