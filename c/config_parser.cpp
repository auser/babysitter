/** includes **/
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <sys/types.h>
#include "config_parser.h"

int ConfigParser::parse_line(const char *line, int len) {
  if (len < 0) {
    return -1;
  } else if (line[0] == '#') {
    // We are in a comment
    return 0;
  } else {
    printf("line: %s", line);
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