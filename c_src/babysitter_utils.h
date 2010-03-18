#ifndef BABYSITTER_UTILS_H
#define BABYSITTER_UTILS_H

#include <string.h>
#include <string>


//---
// Functions
//---
const char *parse_sha_from_git_directory(std::string root_directory);
int babysitter_system_error(int err, const char *fmt, ...);

int debug(const long int cur_level, int level, const char *fmt, ...);

#endif