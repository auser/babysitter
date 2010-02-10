#ifdef __cplusplus

extern "C" {
  int yyparse(void);
  int yylex(void);  
  int yywrap() { return 1; }
}

#else

extern int yylex(void);
extern int yyparse(void);
extern int yywrap(void);
int yyerror(const char *str);

#endif