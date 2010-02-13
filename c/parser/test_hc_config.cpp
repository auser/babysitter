#include <stdio.h>
#include <string.h>

#include "honeycomb_config.h"
#include "hc_support.h"

void usage(const char *progname) {
  fprintf(stderr, "Usage: %s <filename>\n", progname);
  exit(-1);
}

extern FILE *yyin;

int main (int argc, char const *argv[])
{
  if (!argv[1]) usage(argv[0]);
  
  char *filename = strdup(argv[1]);
  printf("----- parsing %s -----\n", filename);
  
  // open a file handle to a particular file:
	FILE *fd = fopen(filename, "r");
	// make sure it's valid:
	if (!fd) {
    fprintf(stderr, "Could not open the file: '%s'\nCheck the permissions and try again\n", filename);
		return -1;
	}
	// set lex to read from it instead of defaulting to STDIN:
	yyin = fd;
	
	// Clear out the config struct for now
  honeycomb_config config = *a_new_honeycomb_config_object();
  
	// parse through the input until there is no more:
  yyparse ((void *) &config);
  
  int i = 0;
  printf("------ output ------\n");
  printf("num_phases: %d (%p)\n", (int)config.num_phases, &config);
  printf("------ phases ------\n");
  for (i = 0; i < (int)config.num_phases; i++) {
    printf("Phase: --- %s ---\n", phase_type_to_string(config.phases[i]->type));
    if (config.phases[i]->before) printf("Before -> %s\n", config.phases[i]->before);
    printf("Command -> %s\n", config.phases[i]->command);
    if (config.phases[i]->after) printf("After -> %s\n", config.phases[i]->after);
    printf("\n");
  }
  
  printf("\n");
  free_config(&config);
  return 0;
}