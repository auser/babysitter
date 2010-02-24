#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <libgen.h>
#include <string>

#include "honeycomb_config.h"
#include "hc_support.h"
#include "babysitter.h"

#ifdef __cplusplus

extern FILE *yyin;
std::string files[BUF_SIZE];
extern ConfigMapT known_configs;
std::string current_file;
int config_file_count = 0;
const char *conf_ext = ".conf";
unsigned int conf_len = strlen(conf_ext);

int yywrap() {
  if (config_file_count < 0) return 1;
  current_file = files[config_file_count];
  const char *filename = current_file.c_str();
  printf("file: %s at %d\n", filename, config_file_count);
    // open a file handle to a particular file:
  FILE *fd = fopen(filename, "r");
  // make sure it's valid:
  if (!fd) {
      fprintf(stderr, "Could not open the file: '%s'\nCheck the permissions and try again\n", filename);
   return NULL;
  }
  // set lex to read from it instead of defaulting to STDIN:
  fclose(yyin);
  yyin = fd;
  
  // Clear out the config struct for now
  honeycomb_config *config = a_new_honeycomb_config_object();
  // Set the filepath on the config
  add_attribute(config, T_FILEPATH, (char *)filename);
  
  debug(DEBUG_LEVEL, 4, "- parsing config file: %s\n", filename);
  char *basefilename = basename((char *)filename);
  unsigned int len = strlen(basefilename);
  
  char *name = (char *) malloc (sizeof(char) * (len - conf_len));
  memcpy(name, basefilename, (len - conf_len));
  
  debug(DEBUG_LEVEL, 4, "- storing in known_configs[%s] = %s\n", name, filename);
  
  known_configs[name] = config;
  
  config_file_count--;
  printf("end of parsing: %s - %d\n", filename, config_file_count);
  return 0;
}

honeycomb_config *parse_config_file(const char* filename) {  
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
  char *fp = strdup(filename);
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
int parse_config_dir(std::string directory, ConfigMapT &known_configs) {  
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
      parse_config_dir(next_dir, known_configs);
    } else {
      unsigned int len = strlen(dir->d_name);
      if (len >= strlen(conf_ext)) {
        if (strcmp(conf_ext, dir->d_name + len - conf_len) == 0) {
          // Copy the name parsed config file into the known_configs
          // We don't want to leak memory, do we?
          char *name = (char *) malloc (sizeof(char) * (len - conf_len));
          memcpy(name, dir->d_name, (len - conf_len));
          std::string file = (directory + dir->d_name);
          printf("Found file: %s [%d]\n", file.c_str(), config_file_count);
          files[config_file_count++] = file;
        }
      }
    }
  }
  closedir( d );
  std::string first_file = files[--config_file_count];
  printf("First file: %s [%d]\n", first_file.c_str(), config_file_count);
  
  // Clear out the config struct for now
  honeycomb_config *config = a_new_honeycomb_config_object();
  // Set the filepath on the config
  add_attribute(config, T_FILEPATH, (char *)first_file.c_str());
  
  if ( (yyin = fopen(first_file.c_str(),"r")) == 0 ) {
    perror(first_file.c_str()); 
    exit(1);
  }
  while (yylex()) ;
  return 0;
}

#endif