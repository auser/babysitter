/** includes **/
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <sys/types.h>
#include "config_parser.h"

ConfigDefinition ConfigParser::parse_config_line(const char *orig_line, int orig_len) {
  ConfigDefinition cd;
  char _namespace[BUFFER], _action[BUFFER], _hooks[BUFFER], _exec[BUFFER];
  int curr_pos = 0;
  char line[BUFFER];
  int len = 0;
  
  memset(_namespace, 0, BUFFER);
  memset(_action, 0, BUFFER);
  memset(_hooks, 0, BUFFER);
  memset(_exec, 0, BUFFER);
  
  // Iterate through the entire string and remove all spaces
  for (int i = 0; i < orig_len; i++) {
    
  }
  // Get the namespace
  for (int i = 0; ((i <= len) && (line[i] != SPLIT_CHAR) && (line[i] != '.')); i++) {
    _namespace[curr_pos++] = line[i];
  }
  _namespace[curr_pos++] = 0;
  for (int i = curr_pos; ((i <= len) && (line[i] != SPLIT_CHAR) && (line[i] != '.')); i++) {
    _action[(i-curr_pos)] = line[i];
  }
  _action[curr_pos++] = 0;
  curr_pos = curr_pos + strlen(_action);
  for (int i = curr_pos; ((i <= len) && (line[i] != SPLIT_CHAR) && (line[i] != '.')); i++) {
    _hooks[(i-curr_pos)] = line[i];
  }
    
  printf("namespace: %s\naction: %s\nhooks: %s\n", _namespace, _action, _hooks);
  cd.set_name(line);
  
  return cd;
}
int ConfigParser::parse_line(const char *line, int len) {
  if (len < 0) {
    return -1;
  } else if (line[0] == '#') {
    // We are in a comment
    return 0;
  } else {
    ConfigDefinition cd = parse_config_line(line, len);
    m_definitions.insert(std::pair<std::string, config_definition>(cd.name(), cd));
    printf("line: %s", cd.name().c_str());
  }
  return 0;
}
int ConfigParser::parse_file(std::string filename) {
  FILE *fp; // File pointer to the filename
  char line[BUFFER];
  int linenum = 0;
  int len;
  
  if (filename == "") {
    fprintf(stderr, "[ConfigParser] Could not open an empty file\n");
    return -1;
  }
  // Open the file
  if ((fp = fopen(filename.c_str(), "r")) == NULL) {
    fprintf(stderr, "[ConfigParser] Could not open '%s' %s\n", filename.c_str(), strerror(errno));
    return -1;
  }
  
  // Setup the lines
  memset(line, 0, BUFFER);
  
  while ( fgets(line, BUFFER, fp) != NULL ) {
    linenum++;
    len = (int)strlen(line);
    
    // Chomp line
    while ((len>=0) && ((line[len]=='\n') || (isspace(line[len])))) {
      line[len]=0;
      len--;
    }
    
    // Skip new lines
    if (line[0] == '\n') continue;
    // Parse the line into the m_dictionary
    parse_line(line, len);
  }
  return 0;
}