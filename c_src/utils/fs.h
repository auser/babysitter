#ifndef FS_H_FILE
#define FS_H_FILE

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include <string>

#include "printing_utils.h"

// Die if we can't chdir to a new path.
void xchdir(const char *path);
// Recursively compute the size of a directory
int dir_size_r(const char *fn);
// Make the path and all parent directories
int mkdir_p(std::string dir, uid_t user, gid_t group, mode_t mode);
int mkdir_p(std::string dir);
// Recursively remove a directory
int rmdir_p(std::string directory);

#endif