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

int debug(int level, char *fmt, ...);
char *phase_type_to_string(phase_type t);
char *attribute_type_to_string(attr_type t);

char *collect_to_period(char *str);
honeycomb_config* new_config();
int add_phase(honeycomb_config *c, phase *p);
phase* new_phase(phase_type t);
void free_config(honeycomb_config *c);
void free_phase(phase *p);