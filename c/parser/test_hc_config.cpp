#include <stdio.h>
#include <string.h>

#include "honeycomb_config.h"
#include "c_ext.h"

int main (int argc, char const *argv[])
{
  printf("----- parsing -----\n");
  yyparse();
  printf("\n");
  return 0;
}