#include <stdio.h>

#include "config_parser.h"

#define ERR -1

int main(int argc, char **argv) {
  ConfigParser cp;  
  
  if (argv[1]) {
    cp.parse_file(argv[1]);
    printf("Do something with the config parser here\n");
  }
  return 0;
}
