/** includes **/
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <sys/types.h>
#include "config_parser.h"

ConfigDefinition ConfigParser::parse_config_line(const char *line, int len, int linenum) {
  ConfigDefinition cd;
 
  int curr_pos = 0;
  int pos = 0;
  int i = 0;
  char dummy[BUFFER];
  int state = 0;
  bool found = false;

  memset(dummy, 0, BUFFER);
  
  do {
    if (line[i] == SPLIT_CHAR) found = true;
    
    if ((line[i] != '.') && (line[i] != SPLIT_CHAR)) {
      if (!isspace(line[i])) {
        dummy[pos++] = line[i];
      }
    } else {
      dummy[pos+1] = 0;
#ifdef DEBUG
      printf("%i = %s\n", state, dummy);
#endif
      switch(state) {
        case 0: 
          cd.set_namespace(dummy);
          break;
        case 1:
          cd.set_action(dummy);
          break;
        case 2:
          if (strcmp(dummy, "before") == 0)
            cd.set_before(dummy);
          else if (strcmp(dummy, "after") == 0)
            cd.set_after(dummy);
          else {
            fprintf(stderr, "Config error: [line %i] %s\nHook %s not supported. The only supported hooks are before and after. Please check your syntax.", linenum, line, dummy);
            exit(-1);
          }
          break;
        default:
          // Should never get here
          fprintf(stderr, "Hey! How in the world did you get here (linenum: %i)? Please report this bug.\n", linenum);
          exit(-1);
      }
      memset(dummy, 0, BUFFER);
      state++; pos = 0;
    }
    curr_pos++;
    i++;
  } while ((i < len) && (!found));

  pos=0;curr_pos++;
  // we are past the split_char
  while (curr_pos <= len) {
    dummy[pos++] = line[curr_pos++];
  }
  dummy[pos+1] = 0;
  
  cd.set_command(dummy);
  cd.finish();

#ifdef DEBUG  
  printf("----%s----\n", line);
  printf("Config definition:\n");
  printf("\tname: '%s'\n", cd.name().c_str());
  printf("\taction: '%s'\n", cd.action().c_str());
  printf("\tbefore: '%s'\n", cd.before().c_str() );
  printf("\tafter: '%s'\n", cd.after().c_str());
  printf("\tcommand: '%s'\n", cd.command().c_str());
  printf("-------\n");
#endif

  return cd;
}
int ConfigParser::parse_line(const char *line, int len, int linenum) {
  if (len < 0) {
    return -1;
  } else if (line[0] == '#') {
    // We are in a comment
    return 0;
  } else {
    ConfigDefinition parsed_config_line = parse_config_line(line, len, linenum);
    std::string action_name (parsed_config_line.name());
		
		if (m_definitions.count(action_name) > 0) {
			// Update existing config definition object
			if(parsed_config_line.before() != "") {
				m_definitions[action_name].set_before((parsed_config_line.command()));
			} else if ((parsed_config_line.after() != "")) {
				m_definitions[action_name].set_after((parsed_config_line.command()));
			} else {
				m_definitions[action_name].set_command((parsed_config_line.command()));
			}
		} else {
			// Insert new config definition object
			if(parsed_config_line.before() != "") {
				parsed_config_line.set_before(parsed_config_line.command());
			} else if ((parsed_config_line.after() != "")) {
				parsed_config_line.set_after(parsed_config_line.command());
			} else {
				parsed_config_line.set_command(parsed_config_line.command());
			}
			// insert	
			m_definitions.insert(std::pair<std::string, config_definition>(action_name, parsed_config_line));
		}
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
    len = (int)strlen(line)-1;
    
    // Chomp the beginning of the line
    for(int i = 0; i < len; ++i) {
      if ( isspace(line[i]) ) {        
        for (int j = 0; j < len; ++j) {
          line[j] = line[j+1];
        }
        line[len--] = 0;
      } else {
        break;
      }
    }
    // Chomp the end of the line
    while ((len>=0) && (isspace(line[len])) ) {
      line[len] = 0;
      len--;
    }
    
    // Skip new lines
    if (line[0] == '\n') continue;
    // Parse the line into the m_dictionary
    parse_line(line, len, linenum);
  }

	// Now we have all the lines in a config_definition instance in the m_dictionary
  return 0;
}

// Dump for debugging
void ConfigDefinition::dump() {
  printf("- %s -\n", m_name.c_str());
	printf("\t- %s\n", m_before.c_str());
	printf("\t- %s\n", m_command.c_str());
	printf("\t- %s\n", m_after.c_str());
}

void ConfigParser::dump() {
  std::map<std::string, config_definition>::iterator it;
	
	for (it = m_definitions.begin(); it != m_definitions.end(); it++) {
		ConfigDefinition cd = it->second;
    cd.dump();
	}
}

ConfigDefinition* ConfigParser::find_config_for(std::string config_name) {
  std::map<std::string, config_definition>::iterator it;
	
	for (it = m_definitions.begin(); it != m_definitions.end(); it++) {
    if (it->first == config_name) return &(it->second);
	}
  return NULL;
}