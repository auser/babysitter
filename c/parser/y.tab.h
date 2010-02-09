#define PHASE 257
#define HOOK 258
#define WORD 259
#define LINE 260
#define SEMICOLON 261
#define QUOTE 262
#define OBRACE 263
#define EBRACE 264
#define WHITESPACE 265
#define COLON 266
#define PERIOD 267
typedef union {
  int i; 
  char* str;
} YYSTYPE;
extern YYSTYPE yylval;
