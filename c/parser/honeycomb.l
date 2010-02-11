%{
#include <stdio.h>
#include <string.h>

#include "honeycomb_config.h"
#include "c_ext.h"
#include "y.tab.h"

#define BUF_SIZE 1024

char buf[BUF_SIZE]; 
char **str_arr;
char *s;
int i, len, str_arr_sz;
%}
/* Keywords */
KEYWORDS    (bundle|start|stop|mount|unmount|cleanup)
RESERVED    (root_dir|directories|executables|env|stdout|stdin)

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

{WHITESPACE}			    {debug(4, "Whitespace: %s\n", yytext);};		/* ignore white space */

  /* Hooks */
{KEYWORDS}\.before  {
                      /* strip off the .before (gross) */
                      yytext = collect_to_period(yytext);
                      debug(3, "next before hook: %s\n", yytext); 
                      return BEFORE;
                    }
{KEYWORDS}\.after   {
                      /* strip off the .after (gross) */
                      yytext = collect_to_period(yytext);
                      debug(3, "next after hook: %s\n", yytext); 
                      return AFTER;
                    }
  /* Keywords */
{KEYWORDS}          debug(3, "next phase: %s\n", yytext); return KEYWORD;
{RESERVED}          {debug(3, "encountered reserved word: %s\n", yytext); return RESERVED;}

{ENDL}                    {debug(4, "Returning new line\n");yylineno++;return ENDL;}
  /* Strings */
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
  debug(3, "returning STRING '%s'\n", buf);
  return STRING;
}
<QUOTEDSTR>\n           {fprintf(stderr, "Error: %4d\tInvalid string\n", yylineno); BEGIN 0;}
<QUOTEDSTR>.            {*s++ = *yytext;}

{STRING}({WHITESPACE}?({STRING}))*  {
                    	yylval.stype = strdup(yytext);
                    	debug(3, "returning STRING: '%s'\n", yytext);
                    	return STRING;
                    }

  /* blocks */
\{                  {
  BEGIN BLOCK;
  str_arr = malloc(1 * sizeof(char *));
  str_arr_sz = 0;
  memset(buf, 0, BUF_SIZE);
  debug(3, "In new block at %d\n", yylineno);
}
<BLOCK>{COMMENT}{ANYTHING}{ENDL}    ; /* ignore comments */
<BLOCK>\}           {
  BEGIN INITIAL;
  int j = 0;
  for (i = 0; i < str_arr_sz; i++) {
    j += strlen(str_arr[i]);
  }
  char **arr = malloc(sizeof(char *) * str_arr_sz);
  memcpy(arr, str_arr, j+1);
  yyleng = str_arr_sz;
  return BLOCK_SET;
}
<BLOCK>\n           {
  ++yylineno;
  debug(4, "%4d\tnewline in block with str: %s\n", yylineno, buf);
  if (str_arr != NULL) {
    str_arr[str_arr_sz] = malloc(sizeof(buf));
    if (str_arr[0] != NULL) {
      strcpy(str_arr[str_arr_sz], buf);
      str_arr_sz++;
    }
  }
  /* reset */
  memset(buf, 0, BUF_SIZE);
  s = buf;
}
<BLOCK>.            {
  *s++ = *yytext;
}

  /* everything else */
{COMMENT}{ANYTHING}{ENDL} {debug(4, "%4d\tIgnoring comment: %s", ++yylineno, yytext);} /* ignore comments */
{EMPTYLINE}               ++yylineno;  /* ignore */ 

%%
