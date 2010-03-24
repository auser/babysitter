#ifndef lint
static char const 
yyrcsid[] = "$FreeBSD: src/usr.bin/yacc/skeleton.c,v 1.28 2000/01/17 02:04:06 bde Exp $";
#endif
#include <stdlib.h>
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYLEX yylex()
#define YYEMPTY -1
#define yyclearin (yychar=(YYEMPTY))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING() (yyerrflag!=0)
static int yygrowstack();
#define YYPREFIX "yy"
#line 2 "honeycomb.y"
#include <stdio.h>
#include <string.h>

#include "honeycomb_config.h"
#include "hc_support.h"
#include "print_utils.h"

extern int yylineno;
extern char *current_parsed_file;
#line 13 "honeycomb.y"
typedef union {
  int i; 
  char* stype;
  char ctype;
  phase_type ptype;
  phase *phase;
  attr_type atype;
} YYSTYPE;
#line 36 "y.tab.c"
#define YYERRCODE 256
#define KEYWORD 257
#define NULLABLE 258
#define BEFORE 259
#define AFTER 260
#define STRING 261
#define ENDL 262
#define BLOCK_SET 263
#define RESERVED 264
const short yylhs[] = {                                        -1,
    0,    0,    7,    7,    7,    7,    4,    4,    4,    3,
    3,    5,    5,    5,    5,    2,    2,    6,    1,    1,
};
const short yylen[] = {                                         2,
    2,    1,    1,    1,    1,    1,    2,    2,    2,    2,
    1,    3,    3,    3,    3,    3,    3,    1,    2,    1,
};
const short yydefred[] = {                                      0,
    0,    0,    0,    0,    6,    0,    5,    0,    3,    4,
    2,   10,    0,    0,    0,    1,    9,   20,   18,    0,
    8,    0,   13,    0,   15,   17,    0,   19,
};
const short yydgoto[] = {                                       6,
   20,    7,    8,    9,   10,   21,   11,
};
const short yysindex[] = {                                    -10,
  -54,  -48,  -43,  -38,    0,  -10,    0, -255,    0,    0,
    0,    0, -245, -245, -256,    0,    0,    0,    0, -234,
    0, -234,    0, -234,    0,    0, -234,    0,
};
const short yyrindex[] = {                                      0,
 -249,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
    0,    7,    0,   13,    0,    0,   19,    0,
};
const short yygindex[] = {                                      0,
   11,    0,    0,    0,    0,    8,   22,
};
#define YYTABLESIZE 283
const short yytable[] = {                                       5,
    7,   26,   17,   12,   18,   18,   12,   19,   11,   13,
    7,   11,   14,   11,   14,   18,   12,   19,   16,   15,
   23,   25,   14,   22,   24,   27,   28,   16,   16,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    1,    0,    2,    3,
    0,    0,    0,    4,    0,    0,    0,    7,    0,    7,
    7,    0,    0,   12,    7,   12,   12,    0,    0,   14,
   12,   14,   14,    0,    0,   16,   14,   16,   16,    0,
    0,    0,   16,
};
const short yycheck[] = {                                      10,
    0,  258,  258,   58,  261,  261,    0,  263,  258,   58,
   10,  261,    0,  263,   58,  261,   10,  263,    0,   58,
   13,   14,   10,   13,   14,   15,  261,    6,   10,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  257,   -1,  259,  260,
   -1,   -1,   -1,  264,   -1,   -1,   -1,  257,   -1,  259,
  260,   -1,   -1,  257,  264,  259,  260,   -1,   -1,  257,
  264,  259,  260,   -1,   -1,  257,  264,  259,  260,   -1,
   -1,   -1,  264,
};
#define YYFINAL 6
#ifndef YYDEBUG
#define YYDEBUG 1
#endif
#define YYMAXTOKEN 264
#if YYDEBUG
const char * const yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,"'\\n'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"':'",0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"KEYWORD","NULLABLE",
"BEFORE","AFTER","STRING","ENDL","BLOCK_SET","RESERVED",
};
const char * const yyrule[] = {
"$accept : program",
"program : program decl",
"program : decl",
"decl : phase",
"decl : hook",
"decl : attr",
"decl : '\\n'",
"phase : phase_decl line",
"phase : phase_decl block",
"phase : phase_decl NULLABLE",
"phase_decl : KEYWORD ':'",
"phase_decl : KEYWORD",
"hook : BEFORE ':' line",
"hook : BEFORE ':' block",
"hook : AFTER ':' line",
"hook : AFTER ':' block",
"attr : RESERVED ':' line",
"attr : RESERVED ':' NULLABLE",
"block : BLOCK_SET",
"line : line STRING",
"line : STRING",
};
#endif
#if YYDEBUG
#include <stdio.h>
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH 10000
#endif
#endif
#define YYINITSTACKSIZE 200
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short *yyss;
short *yysslim;
YYSTYPE *yyvs;
int yystacksize;
#line 153 "honeycomb.y"

int yyerror(const char *str)
{
  fprintf(stderr, "Parsing error %s [%i]: %s\n", current_parsed_file, yylineno, str);
  return 0;
}
#line 212 "y.tab.c"
/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack()
{
    int newsize, i;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = yystacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;
    i = yyssp - yyss;
    newss = yyss ? (short *)realloc(yyss, newsize * sizeof *newss) :
      (short *)malloc(newsize * sizeof *newss);
    if (newss == NULL)
        return -1;
    yyss = newss;
    yyssp = newss + i;
    newvs = yyvs ? (YYSTYPE *)realloc(yyvs, newsize * sizeof *newvs) :
      (YYSTYPE *)malloc(newsize * sizeof *newvs);
    if (newvs == NULL)
        return -1;
    yyvs = newvs;
    yyvsp = newvs + i;
    yystacksize = newsize;
    yysslim = yyss + newsize - 1;
    return 0;
}

#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab

#ifndef YYPARSE_PARAM
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG void
#define YYPARSE_PARAM_DECL
#else	/* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif	/* ANSI-C/C++ */
#else	/* YYPARSE_PARAM */
#ifndef YYPARSE_PARAM_TYPE
#define YYPARSE_PARAM_TYPE void *
#endif
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG YYPARSE_PARAM_TYPE YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else	/* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL YYPARSE_PARAM_TYPE YYPARSE_PARAM;
#endif	/* ANSI-C/C++ */
#endif	/* ! YYPARSE_PARAM */

int
yyparse (YYPARSE_PARAM_ARG)
    YYPARSE_PARAM_DECL
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register const char *yys;

    if ((yys = getenv("YYDEBUG")))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    if (yyss == NULL && yygrowstack()) goto yyoverflow;
    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if ((yyn = yydefred[yystate])) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yysslim && yygrowstack())
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#if defined(lint) || defined(__GNUC__)
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#if defined(lint) || defined(__GNUC__)
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yysslim && yygrowstack())
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 1:
#line 41 "honeycomb.y"
{}
break;
case 2:
#line 42 "honeycomb.y"
{}
break;
case 3:
#line 46 "honeycomb.y"
{debug(DEBUG_LEVEL, 2, "Found phase in program: %p at %d\n", yyvsp[0].phase, yylineno);}
break;
case 4:
#line 47 "honeycomb.y"
{debug(DEBUG_LEVEL, 2, "Found a hook in the program at %d\n", yylineno);}
break;
case 5:
#line 48 "honeycomb.y"
{debug(DEBUG_LEVEL, 2, "Found new attribute in program at: %d\n", yylineno);}
break;
case 6:
#line 49 "honeycomb.y"
{++yylineno;}
break;
case 7:
#line 53 "honeycomb.y"
{
    debug(DEBUG_LEVEL, 2, "Found phase\n");
    /* Set the phase and attach it to the config object*/
    phase *p = find_or_create_phase(config, yyvsp[-1].ptype);
    p->command = (char *) malloc( sizeof(char *) * strlen(yyvsp[0].stype) );
    memset(p->command, 0, strlen(yyvsp[0].stype));
    memcpy(p->command, yyvsp[0].stype, strlen(yyvsp[0].stype));
    add_phase(config, p);
  }
break;
case 8:
#line 62 "honeycomb.y"
{
    /* I think these two can be combined... I hate code duplication*/
    phase *p = find_or_create_phase(config, yyvsp[-1].ptype);
    p->command = (char *) malloc( sizeof(char *) * strlen(yyvsp[0].stype) );
    memset(p->command, 0, strlen(yyvsp[0].stype));
    memcpy(p->command, yyvsp[0].stype, strlen(yyvsp[0].stype));
    free(yyvsp[0].stype);
    /* debug(DEBUG_LEVEL, 3, "Found a phase: [%s %s]\n", phase_type_to_string(p->type), p->command);*/
    add_phase(config, p);
  }
break;
case 9:
#line 72 "honeycomb.y"
{
    debug(DEBUG_LEVEL, 3, "Found a nullable phase_decl: %s\n", phase_type_to_string(yyvsp[-1].ptype));
    phase *p = find_or_create_phase(config, yyvsp[-1].ptype);
    add_phase(config, p);
  }
break;
case 10:
#line 80 "honeycomb.y"
{yyval.ptype = str_to_phase_type(yyvsp[-1].stype); free(yyvsp[-1].stype);}
break;
case 11:
#line 81 "honeycomb.y"
{yyval.ptype = str_to_phase_type(yyvsp[0].stype); free(yyvsp[0].stype);}
break;
case 12:
#line 87 "honeycomb.y"
{
    debug(DEBUG_LEVEL, 3, "Found a hook phrase: %s (%s)\n", yyvsp[0].stype, yyvsp[-2].stype);
    phase_type t = str_to_phase_type(yyvsp[-2].stype);
    /* Do some error checking on the type. please*/
    phase *p = find_or_create_phase(config, t);
    p->before = (char *)malloc(sizeof(char *) * strlen(yyvsp[0].stype));
    memset(p->before, 0, strlen(yyvsp[0].stype));
    memcpy(p->before, yyvsp[0].stype, strlen(yyvsp[0].stype));
    add_phase(config, p);
  }
break;
case 13:
#line 97 "honeycomb.y"
{
    debug(DEBUG_LEVEL, 3, "Found a hook phrase: %s (%s)\n", yyvsp[0].stype, yyvsp[-2].stype);
    phase_type t = str_to_phase_type(yyvsp[-2].stype);
    /* Do some error checking on the type. please*/
    phase *p = find_or_create_phase(config, t);
    p->before = (char *)malloc(sizeof(char *) * strlen(yyvsp[0].stype));
    memset(p->before, 0, strlen(yyvsp[0].stype));
    memcpy(p->before, yyvsp[0].stype, strlen(yyvsp[0].stype));
    add_phase(config, p);
  }
break;
case 14:
#line 107 "honeycomb.y"
{
    debug(DEBUG_LEVEL, 3, "Found a hook phrase: %s (%s)\n", yyvsp[0].stype, yyvsp[-2].stype);
    phase_type t = str_to_phase_type(yyvsp[-2].stype);
    /* Do some error checking on the type. please*/
    phase *p = find_or_create_phase(config, t);
    p->after = (char *)malloc(sizeof(char *) * strlen(yyvsp[0].stype));
    memset(p->after, 0, strlen(yyvsp[0].stype));
    memcpy(p->after, yyvsp[0].stype, strlen(yyvsp[0].stype));
    add_phase(config, p);
  }
break;
case 15:
#line 117 "honeycomb.y"
{
    debug(DEBUG_LEVEL, 3, "Found a hook phrase: %s (%s)\n", yyvsp[0].stype, yyvsp[-2].stype);
    phase_type t = str_to_phase_type(yyvsp[-2].stype);
    /* Do some error checking on the type. please*/
    phase *p = find_or_create_phase(config, t);
    p->after = (char *)malloc(sizeof(char *) * strlen(yyvsp[0].stype));
    memset(p->after, 0, strlen(yyvsp[0].stype));
    memcpy(p->after, yyvsp[0].stype, strlen(yyvsp[0].stype));
    add_phase(config, p);
  }
break;
case 16:
#line 131 "honeycomb.y"
{
    debug(DEBUG_LEVEL, 4, "Found reserved: %d : %s\n", yyvsp[-2].atype, yyvsp[0].stype);
    add_attribute(config, yyvsp[-2].atype, yyvsp[0].stype);
    free(yyvsp[0].stype);
  }
break;
case 17:
#line 136 "honeycomb.y"
{
    debug(DEBUG_LEVEL, 4, "Found empty attribute\n");
  }
break;
case 18:
#line 143 "honeycomb.y"
{debug(DEBUG_LEVEL, 3, "Found a block\n");yyval.stype = yyvsp[0].stype;}
break;
case 19:
#line 148 "honeycomb.y"
{debug(DEBUG_LEVEL, 3, "Found line: '%s'\n", yyvsp[-1].stype);strcpy(yyval.stype,yyvsp[-1].stype);}
break;
case 20:
#line 149 "honeycomb.y"
{debug(DEBUG_LEVEL, 3, "Found line: '%s'\n", yyvsp[0].stype);strcpy(yyval.stype,yyvsp[0].stype);}
break;
#line 550 "y.tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yysslim && yygrowstack())
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
