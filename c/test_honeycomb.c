// Compile:
// make && g++ -o test_honeycomb test_honeycomb.c config_parser.o ei++.o honeycomb.o worker_bee.o -lelf -lei && ./test_honeycomb ../docs/sample_config.conf /tmp/confine

#include <stdio.h>
#include "honeycomb.h"

#define ERR -1

int main(int argc, char **argv) {
  
  std::string config_file ("/etc/beehive/hooks.conf");
  std::string root ("/var/babysitter");
  std::string honeycomb_config ("/etc/beehive/rack.conf");
  mode_t      mode = 040755;
  
  if (argv[1])
    config_file = argv[1];
  if (argv[2])
    root = argv[2];
  if (argv[3])
    honeycomb_config = argv[3];
    
  // config -> in babysitter
  ConfigParser config;
  config.parse_file(config_file);
  
  /**
  * TODO: Change this so it's just text parsing (i.e. file)
  **/
  Honeycomb comb (config);
  printf("---- bundle_environment(%s) ----\n", root.c_str());
  // Extra things
  string_set s_executables;
  // Extra directories to include
  string_set s_dirs;
  s_dirs.insert("/var/lib/gems/1.8/gems/");
  
  string_set s_extra_files;
  // Bundle the environment
  comb.set_honeycomb_config_file(honeycomb_config);
  printf("app_type: %s\n", comb.app_type());
  //comb.parse_honeycomb_config_file(c);
  
  if (comb.bundle_environment(root, mode, s_executables, s_dirs, s_extra_files))
    printf("There was an error!\n");
  printf("-----------\n");
  
  return 0;
}
