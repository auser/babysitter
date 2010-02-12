%option nounput

%{
#include <stdio.h>
#include <string.h>

#include "honeycomb_config.h"
#include "hc_support.h"
#include "y.tab.h"

#define BUF_SIZE 1024

char buf[BUF_SIZE]; 
char **str_arr;
char *s;
int i, len, str_arr_sz;
%}
/* Keywords */
KEYWORDS      (bundle|start|stop|mount|unmount|cleanup)
RESERVEDS     (root_dir|directories|executables|env|stdout|stdin)

/* Others */
COMMENT     #
NL        \n
WHITESPACE  [ \t\r]
STRING  ([a-zA-Z0-9()@<>'"\/.,_-]+)

ANYTHING  (.*)

%x QUOTEDSTR
%x BLOCK

%%

{WHITESPACE}        ; /* Ignore whitespace */
\n                  {return yytext[0];}
:                   {debug(4, "Found ':'\n"); return yytext[0];} /* Special characters with meaning to the syntax */
(NULL|null)         {debug(2, "Found a null\n"); return NULLABLE;}

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
{KEYWORDS}          {debug(3, "next phase: %s\n", yytext); yylval.stype = strdup(yytext); return KEYWORD;}
{RESERVEDS}         {debug(3, "encountered reserved word: %s\n", yytext); yylval.stype = strdup(yytext); return RESERVED;}

  /* blocks */
\{                  {
  BEGIN BLOCK;
  str_arr = malloc(1 * sizeof(char *));
  str_arr_sz = 0;
  memset(buf, 0, BUF_SIZE);
  debug(3, "In new block at %d\n", yylineno);
}
<BLOCK>{COMMENT}{ANYTHING}{NL}    ; /* ignore comments */
<BLOCK>\}           {
  BEGIN INITIAL;
  int j = 0;
  for (i = 0; i < str_arr_sz; i++) {
    j += strlen(str_arr[i]);
  }
  char **arr = malloc(sizeof(char *) * str_arr_sz);
  memcpy(arr, str_arr, j+1);
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
{COMMENT}{ANYTHING}{NL} {debug(4, "%4d\tIgnoring comment: %s", yylineno++, yytext);} /* ignore comments */  

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
  debug(3, "returning QUOTED STRING '%s'\n", buf);
  return STRING;
}
<QUOTEDSTR>\n           {fprintf(stderr, "Error: %4d\tInvalid string\n", yylineno); BEGIN 0;}
<QUOTEDSTR>.            {*s++ = *yytext;}

{STRING}({WHITESPACE}?({STRING}))*  {
  debug(3, "returning STRING: '%s'\n", yytext);
  yylval.stype = strdup(yytext);
  return STRING;
}

.                       {debug(4, "Char: '%c'\n", yytext[0]);}

%%
