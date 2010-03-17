#ifndef PRINTING_UTILS_H
#define PRINTING_UTILS_H

// Error out and die
void fperror(const char *s,...);
void fperror_and_die(int exit_code, const char *s, ...);

#endif