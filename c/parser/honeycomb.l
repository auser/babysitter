%option nounput
%option noinput

%{
#include <stdio.h>
#include <string.h>

#include "honeycomb_config.h"
#include "hc_support.h"
#include "y.tab.h"

#define BUF_SIZE 1024

char buf[BUF_SIZE]; 
char *s;
int i, len;
%}
/* Keywords */
KEYWORDS      (bundle|start|stop|mount|unmount|cleanup)

/* Others */
COMMENT     #
NL        \n
WHITESPACE  [ \t\r]
STRING  ([a-zA-Z0-9()@\$<>'"\/.,_-]+)

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
                      yylval.stype = collect_to_period(yytext);
                      debug(3, "next before hook: %s\n", yytext); 
                      return BEFORE;
                    }
{KEYWORDS}\.after   {
                      /* strip off the .after (gross) */
                      yylval.stype = collect_to_period(yytext);
                      debug(3, "next after hook: %s\n", yytext); 
                      return AFTER;
                    }
  /* Keywords */
{KEYWORDS}          {
  debug(3, "next phase: %s\n", yytext);
  
  yylval.stype = (char *) malloc(sizeof(char) * strlen(yytext));
  memset(yylval.stype, 0, strlen(yytext)); 
  memcpy(yylval.stype, yytext, strlen(yytext));

  return KEYWORD;
}

directories         {yylval.atype = T_DIRECTORIES; return RESERVED;}
executables         {yylval.atype = T_EXECUTABLES; return RESERVED;}
user                {yylval.atype = T_USER; return RESERVED;}
group               {yylval.atype = T_GROUP; return RESERVED;}
env                 {yylval.atype = T_ENV; return RESERVED;}
stdout              {yylval.atype = T_STDOUT; return RESERVED;}
stdin               {yylval.atype = T_STDIN; return RESERVED;}
root_dir            {yylval.atype = T_ROOT_DIR; return RESERVED;}
run_dir             {yylval.atype = T_RUN_DIR; return RESERVED;}
image               {yylval.atype = T_IMAGE; return RESERVED;}
skel_dir            {yylval.atype = T_SKEL_DIR; return RESERVED;}

  /* blocks */
\{                  {
  memset(buf, 0, BUF_SIZE);
  int hd_len = strlen(SCRIPT_HEADER);
  strncpy(buf, SCRIPT_HEADER, hd_len);
  s = buf+hd_len;
  BEGIN BLOCK;
  debug(3, "In new block at %d (%s)\n", yylineno, buf);
}
<BLOCK>{COMMENT}{ANYTHING}{NL}    ; /* ignore comments */
<BLOCK>\}           {
  *s = 0;
  BEGIN INITIAL;
  yylval.stype = strdup(buf);
  debug(3, "returning block string '%s'\n", buf);
  return BLOCK_SET;
}
<BLOCK>\n           {
  ++yylineno;
  // We need more space for the text
  if (strlen(s) >= BUF_SIZE) {
    int new_len = strlen(buf) * 2;
    char temp_buf[new_len];
    memset(temp_buf, 0, new_len);
    memcpy(temp_buf, buf, strlen(buf));
    *s = *temp_buf;
  }
  *s++ = '\n';
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

%%
