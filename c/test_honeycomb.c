// Compile:
// make && g++ -o test_honeycomb test_honeycomb.c ei++.o honeycomb.o worker_bee.o hc_support.o -lelf -lei && ./test_honeycomb ../docs/sample_config.conf /tmp/confine

#include <stdio.h>
#include "honeycomb_config.h"
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
  
  Honeycomb comb;
  printf("comb: %p\n", &comb);
  
  return 0;
}
