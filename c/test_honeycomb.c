// Compile:
// make && g++ -o test_honeycomb test_honeycomb.c config_parser.o ei++.o honeycomb.o worker_bee.o -lelf -lei && ./test_honeycomb ../docs/sample_config.conf /tmp/confine

#include <stdio.h>
#include "honeycomb.h"

#define ERR -1

int main(int argc, char **argv) {
  
  std::string config_file ("/etc/beehive/hooks.conf");
  std::string root ("/var/babysitter");
  mode_t      mode = 040755;
  
  if (argv[1])
    config_file = argv[1];
  if (argv[2])
    root = argv[2];
    
  // config -> in babysitter
  ConfigParser config;
  config.parse_file(config_file);
  
  Honeycomb comb (config);
  printf("---- bundle_environment(%s) ----\n", root.c_str());
  if (comb.bundle_environment(root, 040755))
    printf("There was an error!\n");
  printf("-----------\n");
  
  return 0;
}
