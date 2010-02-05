// Compile:
// make && g++ -o test_config ../c/test_config.c ../c/config_parser.o && ./test_config ../../docs/sample_config.conf 
#include <stdio.h>
#include "babysitter.h"

#define ERR -1

int main(int argc, char **argv) {
  ConfigParser cp;  
  
  if (argv[1]) {
    cp.parse_file(argv[1]);
    //cp.dump();
  }
  
  ConfigDefinition *cd;
  if (cd = cp.find_config_for("rack.bundle")) cd->dump();
  if (cd = cp.find_config_for("rack.start")) cd->dump();
  
  return 0;
}
