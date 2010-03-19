#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <string>

#include "honeycomb_config.h"
#include "hc_support.h"
#include "print_utils.h"

#ifdef __cplusplus
extern "C" {
#endif
  extern FILE *yyin;
  char *current_parsed_file;
  extern int yylineno;
  extern int yydebug;
#ifdef __cplusplus
}
#endif

int yywrap() {
  return 1;
}


#ifdef __cplusplus

honeycomb_config *parse_config_file(std::string conf_file) {  
  const char *filename = conf_file.c_str();
  // open a file handle to a particular file:
	FILE *fd = fopen(filename, "r");
	// make sure it's valid:
	if (!fd) {
    fprintf(stderr, "Could not open the file: '%s'\nCheck the permissions and try again\n", filename);
		return NULL;
	}
	
  if ( (current_parsed_file = (char *) malloc(sizeof(char *) * strlen(filename))) == NULL ) {
    fprintf(stderr, "Could not allocate a new char. Out of memory\n");
    exit(-1);
  }
  memset(current_parsed_file, 0, BUF_SIZE);
  memcpy(current_parsed_file, conf_file.c_str(), conf_file.length());
	// set lex to read from it instead of defaulting to STDIN:
	yyin = fd;
  yylineno = 0;
	
	// Clear out the config struct for now
  honeycomb_config *config = a_new_honeycomb_config_object();
  // Set the filepath on the config
  char *fp = strdup(conf_file.c_str());
  add_attribute(config, T_FILEPATH, fp);
  free(fp);
	// parse through the input until there is no more:
  yydebug = 0;
  yyparse((void *) config);
  
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
      parse_config_dir(next_dir, known_configs);
    } else {
      unsigned int len = strlen(dir->d_name);
      if (len >= strlen(conf_ext)) {
        if (strcmp(conf_ext, dir->d_name + len - conf_len) == 0) {
          // Copy the name parsed config file into the known_configs
          // We don't want to leak memory, do we?
          char *name = (char *) malloc (sizeof(char) * (len - conf_len));
          memcpy(name, dir->d_name, (len - conf_len));
          
          std::string str_name (name);
          std::string file = (directory + "/" + dir->d_name);
          known_configs[str_name] = parse_config_file(file);
        }
      }
    }
  }
  closedir( d );
  return 0;
}

#endif