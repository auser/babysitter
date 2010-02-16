// Compile:
// make && g++ -o test_babysitter test_babysitter.c config_parser.o ei++.o honeycomb.o hc_support.o worker_bee.o parser/y.tab.o parser/lex.yy.o -lelf -lei -lfl
// make && g++ -o test_babysitter test_babysitter.c config_parser.o ei++.o honeycomb.o hc_support.o worker_bee.o parser/y.tab.o parser/lex.yy.o -lelf -lei -lfl && valgrind --tool=memcheck --track-origins=yes --leak-check=yes ./test_babysitter ~auser/apps

#include <stdio.h>
#include <dirent.h>
#include <string.h>

#include "honeycomb_config.h"
#include "hc_support.h"
#include "babysitter.h"

#define ERR -1

std::string config_file_dir;
extern FILE *yyin;
ConfigMapT   known_configs;         // Map containing all the known application configurations (in /etc/beehive/apps, unless otherwise specified)

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
  
	// parse through the input until there is no more:
  yyparse ((void *) config);
  
  return config;
}

// Parse the config directory for config files
int parse_config_dir(std::string directory) {
  printf("Parsing the config directory: %s\n", config_file_dir.c_str());
  
  DIR           *d;
  struct dirent *dir;
  
  d = opendir( directory.c_str() );
  if( d == NULL ) {
    return 1;
  }
  while( ( dir = readdir( d ) ) ) {
    if( strcmp( dir->d_name, "." ) == 0 || strcmp( dir->d_name, ".." ) == 0 ) continue;

    // If this is a directory
    if( dir->d_type == DT_DIR ) {
      std::string next_dir (directory + "/" + dir->d_name);
      printf("next_dir: %s\n", next_dir.c_str());
      parse_config_dir(next_dir);
    } else {
      std::string file = (directory + "/" + dir->d_name);
      printf("file: %s\n", file.c_str());
      known_configs[file] = parse_config_file(file);
    }
  }
  closedir( d );
  return 0;
}

void setup_defaults() {
  config_file_dir = "/etc/babysitter/apps";
}

int main (int argc, char const *argv[])
{
  setup_defaults();
  
  if (argv[1]) config_file_dir = argv[1];
  
  parse_config_dir(config_file_dir);
  
  for(ConfigMapT::iterator it=known_configs.begin(), end=known_configs.end(); it != end; ++it) {
    std::string f = it->first;
    honeycomb_config *c = it->second;
    printf("------ %s ------\n", f.c_str());
    printf("directories: %s\n", c->directories);
    printf("executables: %s\n", c->executables);
    
    //printf("num_phases: %d (%p)\n", (int)c.num_phases, c);
    // printf("------ phases ------\n");
    // unsigned int i;
    // for (i = 0; i < (int)c.num_phases; i++) {
    //   printf("Phase: --- %s ---\n", phase_type_to_string(c.phases[i]->type));
    //   if (c.phases[i]->before) printf("Before -> %s\n", c.phases[i]->before);
    //   printf("Command -> %s\n", c.phases[i]->command);
    //   if (c.phases[i]->after) printf("After -> %s\n", c.phases[i]->after);
    //   printf("\n");
    // }
  }
  // if (config.directories) printf("\t directories: %s\n", config.directories);
  
  for(ConfigMapT::iterator it=known_configs.begin(), end=known_configs.end(); it != end; ++it) {
    printf("Freeing: %s\n", ((std::string) it->first).c_str());
    free_config((honeycomb_config *) it->second);
  }
  return 0;
}