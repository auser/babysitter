#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>                 // For time function
#include <sys/time.h>             // For timeval struct
#include <string>
#include <map>
#include <deque>
#include <pwd.h>        /* getpwdid */

/* Readline */
#include <readline/readline.h>
// #include <readline/history.h>

#include "string_utils.h"
#include "process_manager.h"
#include "babysitter_types.h"
#include "babysitter_utils.h"
#include "hc_support.h"
#include "callbacks.h"

// Globals
extern MapChildrenT children;
extern PidStatusDequeT exited_children;
extern int run_as_user;
extern bool signaled;
extern int dbg;       // Debug flag

extern std::string config_file_dir;
extern ConfigMapT  known_configs;

#ifndef PROMPT_STR
#define PROMPT_STR "bs$ "
#endif

int handle_ok(int transId, pid_t pid) {
  printf("handle_ok!\n");
  return 0;
}

int handle_pid_status_term(const PidStatusT& stat) {return 0;}
int handle_error_str(int transId, bool asAtom, const char* fmt, ...) {return 0;}
int handle_pid_list(int transId, const MapChildrenT& children) {return 0;}

int handle_command_line(char *a, char *b) {
  return 0;
}

void list_processes()
{
  printf("Pid\tName\tStatus\n-----------------------\n");
  if (children.size() == 0) {
    return;
  }
  for(MapChildrenT::iterator it=children.begin(); it != children.end(); it++) 
    printf("%d\t%s\t%s\n",
      it->first, 
      it->second.cmd(),
      it->second.status()
    );
}

void print_help()
{
  printf("Babysitter program help\n"
    "---Commands---\n"
    "h | help             Show this screen\n"
    "s | start <args>     Start a program\n"
    "b | bundle <starts>  Bundle a bee\n"
    "k | kill <args>      Stop a program\n"
    "l | list <args>      List the programs\n"
    "q | quit             Quit the daemon\n"
    "\n"
  );
}

void reload(void *data)
{
  printf("reload");
  debug(dbg, 1, "Reloading configs...\n");
  parse_config_dir(config_file_dir, known_configs); // Parse the config
}

/**
* Setup defaults for the program. These can be overriden after the
* command-line is parsed. This way we don't have variables we expect to be non-NULL 
* being NULL at runtime.
**/
void setup_defaults() {
  pm_setup();
  register_callback("pm_reload", reload);
  config_file_dir = "/etc/beehive/configs";
}

int main (int argc, const char *argv[])
{
  setup_defaults();
  
  if (parse_the_command_line(argc, (char **)argv)) return 0;
  
  const char* env[] = { "PLATFORM_HOST=beehive", NULL };
  int env_c = 1;
  
  // drop_into_shell();  
  // static char *line = (char *)NULL;
  char *cmd_buf;
  int terminated = 0;
  Honeycomb comb;
  
  while (!terminated) {
    // Call the next loop    
    if (pm_next_loop()) break;
    
    // Read the next command
    cmd_buf = readline(PROMPT_STR);
    
    // int result;
    // result = history_expand(line, &cmd_buf);
        
    // if (result < 0 || result == 2) fprintf(stderr, "%s\n", cmd_buf);
    // else { add_history(cmd_buf); }

    cmd_buf[ strlen(cmd_buf) ] = '\0';

    if ( !strncmp(cmd_buf, "quit", 4) || !strncmp(cmd_buf, "exit", 4) ) break; // Get the hell outta here
    
    // Gather args        
    char **command_argv = {0};
    int command_argc = 0;
    if ((command_argc = argify(cmd_buf, &command_argv)) < 1) {
      continue; // Ignore blanks
    }

    if ( !strncmp(command_argv[0], "help", 4) ) {
      print_help();
    } else if ( !strncmp(command_argv[0], "list", 4) ) {
      list_processes();
    } else if ( !strncmp(command_argv[0], "start", 5) ) {
      if (command_argc < 2) {
        fprintf(stderr, "usage: start [command]\n");
      } else {
        // For example: start ./comb_test.sh
        command_argv[command_argc] = 0; // NULL TERMINATE IT
        // pm_start_child(const char* cmd, const char* cd, char* const* env, int user, int nice)
        parse_the_command_line_into_honeycomb_config(command_argc, (char**)command_argv, &comb);
        comb.start();
      }
    } else if ( !strncmp(command_argv[0], "bundle", 5) ) {
      printf("bundle here\n");
    } else if ( !strncmp(command_argv[0], "mount", 5) ) {
      printf("mount here\n");
    } else if ( !strncmp(command_argv[0], "unmount", 7) ) {
      printf("unmount here\n");
    } else if ( !strncmp(command_argv[0], "cleanup", 7) ) {
      printf("cleanup here\n");
    } else if ( !strncmp(command_argv[0], "kill", 4) || !strncmp(command_argv[0], "stop", 4) ) {
      if (command_argc < 2){
        fprintf(stderr, "usage: kill [pid]\n");
      } else {
        pid_t kill_pid = atoi(command_argv[1]);
        time_t now = time (NULL);
        pm_stop_child(kill_pid, 0, now);
      }
    } else if ( !strncmp(command_argv[0], "env", 3)) {
      if (command_argc < 2) {
        printf("-- envs ---\n");
        for(int i = 0; i < env_c; i++)
          printf("\t%s\n", env[i]);
      } else {
        env[env_c++] = strdup(command_argv[1]);
        env[env_c] = NULL;
        printf("adding env: %s\n", command_argv[1]);
      }
    } else {
      printf("Unknown command: %s\ntype 'help' for available commands\n", cmd_buf);
    }
    
    for (int i = 0; i < command_argc; i++) free(command_argv[i]);
    free(cmd_buf);
    // free(line);
  }
  
  printf("Exiting... killing all processes...\n");
  pm_terminate_all();
  return 0;
}
