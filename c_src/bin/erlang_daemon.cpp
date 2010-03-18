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

// globals
long int dbg     = 0;       // Debug flag
ei::Serializer eis(/* packet header size */ 2);

// Required methods
int send_ok(int transId, pid_t pid) {
  printf("send_ok!\n");
  return 0;
}
int send_pid_status_term(const PidStatusT& stat) {return 0;}
int send_error_str(int transId, bool asAtom, const char* fmt, ...) {return 0;}
int send_pid_list(int transId, const MapChildrenT& children) {return 0;}

void usage() {
  fprintf(stderr, "Usage:\n"
  " babysitter [-hnD] [-a N]\n"
  "Options:\n"
  " -h                Show this message\n"
  " -n                Using marshaling file descriptors 3 and 4 instead of default 0 and 1\n"
  " -D                Turn on debugging\n"
  " -C                Directory containing the app configuration files.\n"
  " -u [username]     Run as this user (if running as root)\n"
  " -a [seconds]      Set the number of seconds to live after receiving a SIGTERM/SIGINT (default 30)\n"
  " -b [path]         Path to base all the chroot environments on (not yet functional)\n"
  );
  
  exit(1);
}

int parse_the_command_line(int argc, char* argv[]) {
  /**
  * Command line processing
  **/
  char c;
  while (-1 != (c = getopt(argc, argv, "a:DChn"))) {
    switch (c) {
      case 'h':
      case 'H':
        usage();
        return 1;
      case 'D':
        dbg = true;
        eis.debug(true);
        break;
      case 'C':
        config_file_dir = optarg;
        break;
      case 'a':
        alarm_max_time = atoi(optarg);
        break;
      case 'n':
        eis.set_handles(3,4);
        break;
      case 'u':
        char* run_as_user = optarg;
        struct passwd *pw = NULL;
        if ((pw = getpwnam(run_as_user)) == NULL) {
          fprintf(stderr, "User %s not found!\n", run_as_user);
          exit(3);
        }
        to_set_user_id = pw->pw_uid;
        break;
    }
  }
  return 0;
}

int main (int argc, char const *argv[])
{
  setup_defaults();
  
  const char* env[] = { "PLATFORM_HOST=beehive", NULL };
  int env_c = 1;
  
  parse_the_command_line(argc, argv);
  printf("in erlang main\n");
  return 0;
}