#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <string>
#include <libgen.h>

#include "honeycomb_config.h"
#include "hc_support.h"
#include "babysitter.h"

#ifdef __cplusplus

extern FILE *yyin;
extern ConfigMapT known_configs;
std::list <std::string> *files;
unsigned int current_config_file_count = 0;
const char *conf_ext = ".conf";
unsigned int conf_len = strlen(conf_ext);
std::string current_file;
  
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

int yywrap() {
  fclose(yyin);
  // Setup a new config object
  // Clear out the config struct for now
  honeycomb_config *config = a_new_honeycomb_config_object();
  // Set the filepath on the config
  char *fp = strdup(current_file.c_str());
  add_attribute(config, T_FILEPATH, fp);
  free(fp);
  
  const char *filename = basename(fp);
  unsigned int len = strlen(filename);
  char *name = (char *) malloc (sizeof(char) * (len - conf_len));
  memcpy(name, filename, (len - conf_len));
  
  printf("inserting %p as %s\n", config, name);
  known_configs[name] = config;
  
  if (current_config_file_count > files->size()) return 1;
  std::string file_path = files->front();
  files->pop_front();
  
  printf("Char: %s\n", file_path.c_str());
  if ((yyin = fopen(file_path.c_str(), "r")) == 0) {
    perror(file_path.c_str());
    exit(-1);
  }
  current_file = file_path;
  return 0;
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
      unsigned int len = strlen(dir->d_name);
      if (len >= strlen(conf_ext)) {
        if (strcmp(conf_ext, dir->d_name + len - conf_len) == 0) {
          // Copy the name parsed config file into the known_configs
          // We don't want to leak memory, do we?
          std::string file = (directory + "/" + dir->d_name);
          files->push_back(file);
        }
      }
    }
  }
  closedir( d );  
  
  std::string file_path = files->front();
  files->pop_front();
  
  if ((yyin = fopen(file_path.c_str(), "r")) == 0) {
    perror(file_path.c_str());
    exit(-1);
  }
  
  while( yylex() )
  ;
  return 0;
}

#endif