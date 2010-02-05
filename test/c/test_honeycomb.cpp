// Compile:
// cd ../../ && make && cd test/c && g++ -o test_config ../c/test_config.c ../c/config_parser.o && ./test_config ../../docs/sample_config.conf 
#include <stdio.h>
#include "honeycomb.h"

#define ERR -1

int main(int argc, char **argv) {
  
  if (argv[1]) {
    printf("argv: %s\n", argv[1]);
  }
  
  return 0;
}
