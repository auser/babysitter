#ifndef BABYSITTER_UTILS_H
#define BABYSITTER_UTILS_H

//---
// Functions
//---
const char *parse_sha_from_git_directory(std::string root_directory);
int dir_size_r(const char *fn);
int mkdir_p(std::string dir, uid_t user, gid_t group, mode_t mode);
int rmdir_p(std::string dir);
int babysitter_system_error(int err, const char *fmt, ...);
int argify (const char * input, int * argc_ptr, char *** argv_ptr);

#endif