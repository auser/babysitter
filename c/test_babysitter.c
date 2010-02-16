// Compile:
// make && g++ -o test_babysitter test_babysitter.c config_parser.o ei++.o honeycomb.o hc_support.o worker_bee.o parser/y.tab.o parser/lex.yy.o -lelf -lei -lfl
// make && g++ -o test_babysitter test_babysitter.c ei++.o honeycomb.o hc_support.o worker_bee.o parser/y.tab.o parser/lex.yy.o -lelf -lei -lfl && ./test_babysitter ~auser/apps
 // valgrind --tool=memcheck --track-origins=yes --leak-check=yes ./test_babysitter ~auser/apps

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
  // Set the filepath on the config
  char *fp = strdup(conf_file.c_str());
  add_attribute(config, T_FILEPATH, fp);
  free(fp);
	// parse through the input until there is no more:
  yyparse ((void *) config);
  
  return config;
}

// Parse the config directory for config files
/**
* Parse the config directory for config files (ending in the extension: .conf)
* This will recursively look through the config_file_dir and parse config files
* and place them into the known_configs map with the filename without the extension
* for access later.
**/
int parse_config_dir(std::string directory) {  
  DIR           *d;
  struct dirent *dir;
  const char *conf_ext = ".conf";
  unsigned int conf_len = strlen(conf_ext);
  
  d = opendir( directory.c_str() );
  if( d == NULL ) {
    return 1;
  }
  while( ( dir = readdir( d ) ) ) {
    if( strcmp( dir->d_name, "." ) == 0 || strcmp( dir->d_name, ".." ) == 0 ) continue;

    // If this is a directory
    if( dir->d_type == DT_DIR ) {
      std::string next_dir (directory + "/" + dir->d_name);
      parse_config_dir(next_dir);
    } else {
      int len = strlen(dir->d_name);
      if (len >= strlen(conf_ext)) {
        if (strcmp(conf_ext, dir->d_name + len - conf_len) == 0) {
          // Copy the name parsed config file into the known_configs
          // We don't want to leak memory, do we?
          char *name = (char *) malloc (sizeof(char) * (len - conf_len));
          memcpy(name, dir->d_name, (len - conf_len));
          std::string file = (directory + "/" + dir->d_name);
          printf("storing in known_configs[%s] = %s\n", name, file.c_str());
          known_configs[name] = parse_config_file(file);
        }
      }
    }
  }
  closedir( d );
  return 0;
}

void setup_defaults() {
  config_file_dir = "/etc/babysitter/apps";
}

// honeycomb_config* find_config_for_app_type(std::string str) {
//   known_configs.find(str);
// }

int main (int argc, char const *argv[])
{
  setup_defaults();
  
  if (argv[1]) config_file_dir = argv[1];
  
  parse_config_dir(config_file_dir);
  
  for(ConfigMapT::iterator it=known_configs.begin(), end=known_configs.end(); it != end; ++it) {
    std::string f = it->first;
    honeycomb_config *c = it->second;
    printf("------ %s ------\n", c->filepath);
    if (c->directories != NULL) printf("directories: %s\n", c->directories);
    if (c->executables != NULL) printf("executables: %s\n", c->executables);
    
    printf("------ phases (%d) ------\n", (int)c->num_phases);
    unsigned int i;
    for (i = 0; i < (int)c->num_phases; i++) {
      printf("Phase: --- %s ---\n", phase_type_to_string(c->phases[i]->type));
      if (c->phases[i]->before != NULL) printf("Before -> %s\n", c->phases[i]->before);
      printf("Command -> %s\n", c->phases[i]->command);
      if (c->phases[i]->after != NULL) printf("After -> %s\n", c->phases[i]->after);
      printf("\n");
    }
  }
  // if (config.directories) printf("\t directories: %s\n", config.directories);
  
  for(ConfigMapT::iterator it=known_configs.begin(), end=known_configs.end(); it != end; ++it) {
    printf("Freeing: %s\n", ((std::string) it->first).c_str());
    free_config((honeycomb_config *) it->second);
  }
  return 0;
}