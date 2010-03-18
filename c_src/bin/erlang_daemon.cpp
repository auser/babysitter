#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>                 // For time function
#include <sys/time.h>             // For timeval struct
#include <string>
#include <map>
#include <deque>
#include <pwd.h>        /* getpwdid */

#include <ei.h>
#include "ei++.h"

#include "string_utils.h"
#include "process_manager.h"
#include "honeycomb_config.h"
#include "print_utils.h"
#include "hc_support.h"
#include "babysitter_types.h"

// globals
extern int dbg;       // Debug flag
ei::Serializer eis(/* packet header size */ 2);
extern std::string config_file_dir;
extern ConfigMapT  known_configs;

// Required methods
int send_ok(int transId, pid_t pid) {
  printf("send_ok!\n");
  return 0;
}
int send_pid_status_term(const PidStatusT& stat) {return 0;}
int send_error_str(int transId, bool asAtom, const char* fmt, ...) {return 0;}
int send_pid_list(int transId, const MapChildrenT& children) {return 0;}

int main (int argc, char const *argv[])
{
  setup_process_manager_defaults();
  
  // const char* env[] = { "PLATFORM_HOST=beehive", NULL };
  // int env_c = 1;
  
  if (parse_the_command_line(argc, (char **)argv)) return 0;
  
  debug(dbg, 2, "parsing the config directory: %s\n", config_file_dir.c_str());
  parse_config_dir(config_file_dir, known_configs); // Parse the config
  return 0;
}