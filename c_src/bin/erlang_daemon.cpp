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
  debug(dbg, 4, "erlang daemon got a signal: %d\n", signal);
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
  pid_t pid = pm_start_child((const char*)co.cmd(), co.cd(), (const char**)co.env(), co.user(), co.nice());
  if (pid < 0) {
    fperror("pm_start_child");
    send_error_str(transId, false, "Couldn't start pid: %s", strerror(errno));
    return -1;
  } else {
    CmdInfo ci(co.cmd(), co.kill_cmd(), pid);
    children[pid] = ci;
    send_ok(transId, pid);
    return 0;
  }
}

int ei_decode(std::string cmd, Honeycomb *comb)
{
    // {Cmd::string(), [Option]}
    //      Option = {env, Strings} | {cd, Dir} | {kill, Cmd}
  int sz = 0, nice = 0;
  std::string op, val;
  e_error.str("");
    
  if (eis.decodeString(cmd) < 0) {
    e_error << "badarg: cmd string expected or string size too large";
    return -1;
  } else if ((sz = eis.decodeListSize()) < 0) {
    e_error << "option list expected";
    return -1;
  } else if (sz == 0) {
    comb->set_root_dir("");
    // comb->set_ki
    return 0;
  }

  for ( int i = 0; i < sz; i++) {
    enum OptionT            { CD,   ENV,   NICE,   USER } opt;
    const char* options[] = {"cd", "env", "nice", "user" };
      
    if (eis.decodeTupleSize() != 2 || (int)(opt = (OptionT)eis.decodeAtomIndex(options, op)) < 0) {
      e_error << "badarg: cmd option must be an atom"; return -1;
    }
        
    switch (opt) {
      case CD:
      case USER:
      // {cd, Dir::string()} | {kill, Cmd::string()}
        if (eis.decodeString(val) < 0) {
          e_error << op << " bad option value"; return -1;
        }
        if      (opt == CD)     comb->set_root_dir(val);
        else if (opt == USER) {
          struct passwd *pw = getpwnam(val.c_str());
          if (pw == NULL) {
            e_error << "Invalid user " << val << ": " << ::strerror(errno);
            return -1;
          }
          comb->set_user(pw->pw_uid);
        }
        break;

      case NICE:
        if (eis.decodeInt(nice) < 0 || nice < -20 || nice > 20) {
          e_error << "nice option must be an integer between -20 and 20"; 
          return -1;
        }
        comb->set_nice(nice);
        break;

      case ENV: {
        // {env, [NameEqualsValue::string()]}
        int env_sz = eis.decodeListSize();
        if (env_sz < 0) {
          e_error << "env list expected"; return -1;
        }

        for (int i=0; i < env_sz; i++) {
          std::string s;
          if (eis.decodeString(s) < 0) {
            e_error << "invalid env argument #" << i; return -1;
          }
          comb->add_env(s);
        }
        break;
      }
      default:
        e_error << "bad option: " << op; return -1;
      }
    }

  return 0;
}


int main (int argc, char const *argv[])
{
  fd_set readfds;
  Honeycomb comb;

  // Never use stdin/stdout/stderr
  eis.set_handles(3, 4);
  if (parse_the_command_line(argc, (char **)argv)) return 0;
  
  setup_erl_daemon_signal_handlers();
  setup_process_manager_defaults();
  
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
    debug(dbg, 4, "looping... (%d)\n", (int)terminated);

    // Detect "open" for serial pty slave
    FD_ZERO (&readfds);
    FD_SET (eis.read_handle(), &readfds);
    
    if (pm_next_loop()) break;
    
    // Erlang fun... pull the next command from the readfds parameter on the erlang fd
    ei::TimeVal timeout(5, 0);
    int cnt = select(maxfd, &readfds, (fd_set *)0, (fd_set *) 0, &timeout.timeval());
    int interrupted = (cnt < 0 && errno == EINTR);
      
    if (interrupted || cnt == 0) {
      if (pm_check_children(terminated) < 0) break;
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
      
      /* Our marshalling spec is that we are expecting a tuple {TransId, {Cmd::atom(), Arg1, Arg2, ...}} */
      if (eis.decodeTupleSize() != 2 || (eis.decodeInt(transId)) < 0 || (arity = eis.decodeTupleSize()) < 1) {
        terminated = 10; break;
      }
      
      debug(dbg, 4, "terminated before commands: %d\n", (int) terminated);
      // Available commands from erlang    
      enum CmdTypeT         { BUNDLE,    RUN,   STOP,   KILL,   LIST  } cmd;
      const char* cmds[] =  { "bundle", "run", "stop", "kill", "list" };

      // Determine which of the commands was called
      if ((int)(cmd = (CmdTypeT) eis.decodeAtomIndex(cmds, command)) < 0) {
        if (send_error_str(transId, false, "Unknown command: %s", command.c_str()) < 0) {
          terminated = 10;
          break;
        } else continue;
      }
      
      if (arity != 3 || ei_decode(command, &comb) < 0) {
        send_error_str(transId, false, e_error.str().c_str());
        continue;
      }
      
      switch (cmd) {
        case BUNDLE:
          comb.bundle();
          send_ok(transId, 0);
        break;
        case RUN:
          comb.start();
          send_ok(transId, 0);
        break;
        case STOP:
        case KILL:
          comb.stop();
          // pid_t kill_pid = atoi(command_argv[1]);
          // time_t now = time (NULL);
          // pm_stop_child(kill_pid, 0, now);
          send_ok(transId, 0);
        break;
        case LIST:
          debug(dbg, 1, "listing\n");
          break;
        default: {
          debug(dbg, 4, "got command: %s\n", command.c_str());
          send_ok(transId, 0);
        }
        break;
      }
    }
    eis.reset();
  }
  
  debug(dbg, 2, "Terminating erlang_daemon\n");
  pm_terminate_all();
  return 0;
}

