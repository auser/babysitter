// Compile:
// make && g++ -o test_honeycomb test_honeycomb.c config_parser.o ei++.o -lelf -lei

#include <stdio.h>
#include "honeycomb.h"

#define ERR -1

int main(int argc, char **argv) {
  
  std::string config ("/etc/beehive/hooks.conf");
  std::string root ("/var/babysitter");
  mode_t      mode = 040755;
  
  if (argv[1]) {
    // config -> in babysitter
    ConfigParser config;
    std::string config_file (argv[1]);
    config.parse_file(config_file);
    
    Honeycomb po (config);
    printf("argv: %s\n", argv[1]);
  }
  
  return 0;
}
