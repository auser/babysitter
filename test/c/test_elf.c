// Compile:
// make && g++ -o test_elf test_elf.c worker_bee.o -lelf && ./test_elf /tmp/test && tree /tmp/test

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
  s_executables.insert("rm");
  s_executables.insert("cat");
  
  string_set s_dirs;
  s_dirs.insert("/opt");
  std::string root_path;
  
  string_set s_extra_files;
  
  if (argv[1]) {
    root_path = argv[1];
    printf("-- building chroot: %s\n", root_path.c_str());
    b.build_chroot(root_path, 1000, 1000, s_executables, s_extra_files, s_dirs);
  }
  if (argv[2]) {
    root_path = argv[2];
    printf("-- building base dir: %s\n", root_path.c_str());
    b.build_base_dir(root_path, 1000, 1000);
  }
  return 0;
}
