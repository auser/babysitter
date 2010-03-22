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
extern MapChildrenT children;
extern PidStatusDequeT exited_children;
extern bool signaled;
struct sigaction                mainact;
extern int terminated;

// Required methods
int send_ok(int transId, pid_t pid) {
  printf("send_ok!\n");
  return 0;
}
int send_pid_status_term(const PidStatusT& stat) {return 0;}
int send_error_str(int transId, bool asAtom, const char* fmt, ...) {return 0;}
int send_pid_list(int transId, const MapChildrenT& children) {return 0;}

void erl_d_gotsignal(int signal)
{
  if (signal == SIGTERM || signal == SIGINT || signal == SIGPIPE)
    terminated = 1;
}

void setup_erl_daemon_signal_handlers()
{
  mainact.sa_handler = erl_d_gotsignal;
  sigemptyset(&mainact.sa_mask);
  sigaddset(&mainact.sa_mask, SIGCHLD);
  mainact.sa_flags = 0;
  sigaction(SIGINT,  &mainact, NULL);
  sigaction(SIGTERM, &mainact, NULL);
  sigaction(SIGHUP,  &mainact, NULL);
  sigaction(SIGPIPE, &mainact, NULL);
}

int handle_command_line(char *a, char *b) {
  if (!strncmp(a, "--debug", 7) || !strncmp(a, "-D", 2)) {
    eis.debug(true);
  } else if (!strncmp(a, "--std", 5)) {
    eis.set_handles(1, 2);
  }
  return 0;
}

int main (int argc, char const *argv[])
{
  fd_set readfds;
  setup_process_manager_defaults();
  setup_erl_daemon_signal_handlers();
  // const char* env[] = { "PLATFORM_HOST=beehive", NULL };
  // int env_c = 1;
  // Never use stdin/stdout/stderr
  eis.set_handles(3, 4);
  if (parse_the_command_line(argc, (char **)argv)) return 0;
  
  debug(dbg, 2, "parsing the config directory: %s\n", config_file_dir.c_str());
  parse_config_dir(config_file_dir, known_configs); // Parse the config
  
  const int maxfd = eis.read_handle()+1;
    
  /**
  * Program loop. 
  * Daemonizing loop. This waits for signals
  * from the erlang process
  **/
  debug(dbg, 2, "Entering daemon loop\n");
  while (!terminated) {
    // Detect "open" for serial pty slave
    FD_ZERO (&readfds);
    FD_SET (eis.read_handle(), &readfds);
    
    while (!terminated && (exited_children.size() > 0 || signaled)) check_children(terminated);
    check_pending(); // Check for pending signals arrived while we were in the signal handler
    if (terminated) break;
    
    ei::TimeVal timeout(5, 0);
    int cnt = select (maxfd, &readfds, (fd_set *)0, (fd_set *) 0, &timeout.timeval());
    int interrupted = (cnt < 0 && errno == EINTR);
      
    if (interrupted || cnt == 0) {
      if (check_children(terminated) < 0) break;
    } else if (cnt < 0) {
      perror("select"); 
      exit(9);
    } else if (FD_ISSET (eis.read_handle(), &readfds) ) {
      // Read from fin a command sent by Erlang
      int  err, arity;
      long transId;
      std::string command;
      
      err = eis.read();
      
      // Note that if we were using non-blocking reads, we'd also need to check for errno EWOULDBLOCK.
      if (err < 0) {
        terminated = 90-err;
        break;
      }
      
      fprintf(stderr, "err: %d\n", err);
      
      /* Our marshalling spec is that we are expecting a tuple {TransId, {Cmd::atom(), Arg1, Arg2, ...}} */
      if (eis.decodeTupleSize() != 2 || (eis.decodeInt(transId)) < 0 || (arity = eis.decodeTupleSize()) < 1) {
        terminated = 10; break;
      }
      
      // Available commands from erlang    
      enum CmdTypeT { BUNDLE, EXECUTE, SHELL, STOP, KILL, LIST } cmd;
      const char* cmds[] = {"bundle", "run","shell","stop","kill","list"};

      // Determine which of the commands was called
      if ((int)(cmd = (CmdTypeT) eis.decodeAtomIndex(cmds, command)) < 0) {
        if (send_error_str(transId, false, "Unknown command: %s", command.c_str()) < 0) {
          terminated = 11; 
          break;
        } else continue;
      }

      switch (cmd) {
        default:
        fprintf(stderr, "got command: %s\n", command.c_str());
        break;
      }
    }      
  }
    
  terminate_all();
  return 0;
}