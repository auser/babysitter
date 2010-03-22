#ifndef BABYSITTER_UTILS_H
#define BABYSITTER_UTILS_H

#include <string.h>
#include <string>

#include "honeycomb.h"
#include "honeycomb_config.h"

//---
// Functions
//---
const char *parse_sha_from_git_directory(std::string root_directory);
int babysitter_system_error(int err, const char *fmt, ...);

void version(FILE *fp);
int usage(int c, bool detailed = false);

int parse_the_command_line_into_honeycomb_config(int argc, char **argv, Honeycomb *comb);
int parse_the_command_line(int argc, char *argv[], int c = 0);

#endif
