#ifndef PRINT_UTILS_H
#define PRINT_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif
  int debug(const long int cur_level, int level, const char *fmt, ...);
#ifdef __cplusplus
}
#endif

// Error out and die
void fperror(const char *s,...);
void fperror_and_die(int exit_code, const char *s, ...);

#endif