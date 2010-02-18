// Compile:
// make && g++ -g -o test_honeycomb test_honeycomb.c ei++.o honeycomb.o worker_bee.o hc_support.o ./parser/y.tab.o ./parser/lex.yy.o -lelf -lei && sudo rm -rf /var/beehive/honeycombs/* && ./test_honeycomb ../docs/apps/rack.conf /tmp/confine rack

#include <stdio.h>

#include "honeycomb_config.h"
#include "hc_support.h"
#include "babysitter.h"

#define ERR -1

std::string config_file_dir;
extern FILE *yyin;

/**
* Parse the config file with lex/yacc
**/
honeycomb_config *parse_config_file(std::string conf_file) {  
  const char *filename = conf_file.c_str();
  // open a file handle to a particular file:
	FILE *fd = fopen(filename, "r");
	// make sure it's valid:
	if (!fd) {
    fprintf(stderr, "Could not open the file: '%s'\nCheck the permissions and try again\n", filename);
		return NULL;
	}
	// set lex to read from it instead of defaulting to STDIN:
	yyin = fd;
	
	// Clear out the config struct for now
  honeycomb_config *config = a_new_honeycomb_config_object();
  // Set the filepath on the config
  char *fp = strdup(conf_file.c_str());
  add_attribute(config, T_FILEPATH, fp);
  free(fp);
	// parse through the input until there is no more:
  yyparse ((void *) config);
  
  return config;
}

int main(int argc, char **argv) {
  
  // Defaults
  std::string config_file ("/etc/beehive/hooks.conf");
  std::string root ("/var/babysitter");
  std::string app_type ("rack");
  
  if (argv[1])
    config_file = argv[1];
  if (argv[2])
    root = argv[2];
  if (argv[3])
    app_type = argv[3];
  
  printf("-----\n");
  honeycomb_config *c = parse_config_file(config_file);
    
  Honeycomb comb (app_type, c);
  printf("comb: %p\n", &comb);
  printf("\tuser: %d\n", comb.user());
  printf("\tgroup: %d\n", comb.group());
  printf("\tcd: %s\n", comb.cd());
  printf("\trun_dir: %s\n", comb.run_dir());
  
  printf("\t--executables\n");
  for (string_set::iterator exec = comb.executables().begin(); exec != comb.executables().end(); exec++) printf("\t\t%s\n", exec->c_str());
  
  printf("\t--directories\n");
  for (string_set::iterator dir = comb.directories().begin(); dir != comb.directories().end(); dir++) printf("\t\t%s\n", dir->c_str());
  
  comb.bundle();
  
  return 0;
}
