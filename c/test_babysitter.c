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