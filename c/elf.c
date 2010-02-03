#include <stdio.h>
#include <string.h>
#include <error.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gelf.h>
#include <set>
#include <regex.h>

/** Worker bee **/
#include "worker_bee.h"

#define ERR -1

int main(int argc, char **argv) {
  WorkerBee b;

  string_set s_executables;
  s_executables.insert("/bin/ls");
  s_executables.insert("/bin/bash");
  s_executables.insert("/usr/bin/whoami");
  s_executables.insert("ruby");
  s_executables.insert("touch");
  
  string_set s_dirs;
  s_dirs.insert("/opt");
  
  if (argv[1]) {
    const std::string root_path = argv[1];
    printf("-- building chroot: %s\n", root_path.c_str());
    b.build_chroot(root_path, s_executables, s_dirs);
  }
  return 0;
}
