#ifndef PM_HELPERS_H
#define PM_HELPERS_H

#define DEFAULT_PATH "/bin:/usr/bin:/usr/local/bin:/sbin;"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <paths.h>
#include <errno.h>

/* Defines */
#ifndef MAXATOMLEN
#define MAXATOMLEN 256
#endif

#ifndef BUFFER_SZ
#define BUFFER_SZ 1024
#endif

#ifndef MAX_BUFFER_SZ
#define MAX_BUFFER_SZ 4096
#endif

#ifndef PREFIX_LEN
#define PREFIX_LEN 8
#endif

/* prototypes */
int pm_abs_path(const char *path);
const char *find_binary(const char *file);

#endif
