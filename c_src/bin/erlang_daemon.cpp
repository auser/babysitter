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
#include "command_options.h"
#include "command_info.h"

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
std::stringstream     e_error;

// Required methods

int handle_ok(int transId, pid_t pid) {
  printf("handle_ok!\n");
  return 0;
}

int handle_pid_status_term(const PidStatusT& stat) {return 0;}
int handle_error_str(int transId, bool asAtom, const char* fmt, ...) {return 0;}
int handle_pid_list(int transId, const MapChildrenT& children) {return 0;}

// Erlang methods
int send_ok(int transId, pid_t pid) {
  eis.reset();
  eis.encodeTupleSize(2);
  eis.encode(transId);
  if (pid < 0)
    eis.encode(ei::atom_t("ok"));
  else {
    eis.encodeTupleSize(2);
    eis.encode(ei::atom_t("ok"));
    eis.encode(pid);
  }
  return eis.write();
}
int send_term(int transId, bool asAtom, const char *term, const char* fmt, ...)
{
  char str[MAXATOMLEN];
  va_list vargs;
  va_start (vargs, fmt);
  vsnprintf(str, sizeof(str), fmt, vargs);
  va_end   (vargs);
  
  eis.reset();
  eis.encodeTupleSize(2);
  eis.encode(transId);
  eis.encodeTupleSize(2);
  eis.encode(ei::atom_t(term));
  (asAtom) ? eis.encode(ei::atom_t(str)) : eis.encode(str);
  return eis.write();
}
int send_pid_status_term(const PidStatusT& stat) {
  eis.reset();
  eis.encodeTupleSize(2);
  eis.encode(0);
  eis.encodeTupleSize(3);
  eis.encode(ei::atom_t("exit_status"));
  eis.encode(stat.first);
  eis.encode(stat.second);
  return eis.write();
}
int send_error_str(int transId, bool asAtom, const char* fmt, ...) {
  char str[MAXATOMLEN];
  va_list vargs;
  va_start (vargs, fmt);
  vsnprintf(str, sizeof(str), fmt, vargs);
  va_end   (vargs);
  
  eis.reset();
  eis.encodeTupleSize(2);
  eis.encode(transId);
  eis.encodeTupleSize(2);
  eis.encode(ei::atom_t("error"));
  (asAtom) ? eis.encode(ei::atom_t(str)) : eis.encode(str);
  return eis.write();
}
int send_pid_list(int transId, const MapChildrenT& children) {
  eis.reset();
  eis.encodeTupleSize(2);
  eis.encode(transId);
  eis.encodeListSize(children.size());
  for(MapChildrenT::const_iterator it=children.begin(), end=children.end(); it != end; ++it)
      eis.encode(it->first);
  eis.encodeListEnd();
  return eis.write();
}

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

/**
* Commands
**/
int cmd_start(int transId, CmdOptions& co) {
  pid_t pid = start_child((const char*)co.cmd(), co.cd(), (const char**)co.env(), co.user(), co.nice());
  if (pid < 0) {
    fperror("start_child");
    send_error_str(transId, false, "Couldn't start pid: %s", strerror(errno));
    return -1;
  } else {
    CmdInfo ci(co.cmd(), co.kill_cmd(), pid);
    children[pid] = ci;
    send_ok(transId, pid);
    return 0;
  }
}
int cmd_bundle(int transId, CmdOptions& co) {return 0;}
int cmd_mount(int transId, CmdOptions& co) {return 0;}
int cmd_stop(int transId, CmdOptions& co) {return 0;}
int cmd_kill(int transId, CmdOptions& co) {return 0;}
int cmd_list(int transId, CmdOptions& co) {return 0;}
int cmd_unmount(int transId, CmdOptions& co) {return 0;}
int cmd_cleanup(int transId, CmdOptions& co) {return 0;}

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
      if (err < 0 || err == EWOULDBLOCK) {
        terminated = 90-err;
        break;
      }
      
      /* Our marshalling spec is that we are expecting a tuple {TransId, {Cmd::atom(), Arg1, Arg2, ...}} */
      if (eis.decodeTupleSize() != 2 || (eis.decodeInt(transId)) < 0 || (arity = eis.decodeTupleSize()) < 1) {
        terminated = 10; break;
      }
      
      // Available commands from erlang    
      enum CmdTypeT         { BUNDLE,   MOUNT,     RUN,   STOP,   KILL,   LIST,   UNMOUNT,   CLEANUP } cmd;
      const char* cmds[] =  { "bundle", "mount",  "run", "stop", "kill", "list", "unmount", "cleanup"};

      // Determine which of the commands was called
      if ((int)(cmd = (CmdTypeT) eis.decodeAtomIndex(cmds, command)) < 0) {
        if (send_error_str(transId, false, "Unknown command: %s", command.c_str()) < 0) {
          terminated = 11; 
          break;
        } else continue;
      }
      
      CmdOptions co;
      switch (cmd) {
        case RUN: {
          if (arity != 3 || co.ei_decode(eis) < 0) {
            send_error_str(transId, false, co.strerror());
            continue;
          }
          cmd_start(transId, co);
        }
        break;
        default: {
          fprintf(stderr, "got command: %s\n", command.c_str());
          send_ok(transId, 0);
        }
        break;
      }
      eis.reset();
    }      
  }
    
  terminate_all();
  return 0;
}