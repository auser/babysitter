/**
 * Babysitter -- Mount and run a command in a chrooted environment with mouted image support
 *
 * Based on the brilliant work of snackypants, aka Chris Palmer <chris@isecpartners.com> and Serge Aleynikov
 * Description:
 * ============
 * Erlang port program for spawning and controlling OS tasks. 
 * It listens for commands sent from Erlang and executes them until 
 * the pipe connecting it to Erlang VM is closed or the program 
 * receives SIGINT or SIGTERM. At that point it kills all processes
 * it forked by issuing SIGTERM followed by SIGKILL in 6 seconds.
 * 
 * Marshalling protocol:
 *     Erlang                                                 C++
 *     | ---- {TransId::integer(), Instruction::tuple()} ---> |
 *     | <----------- {TransId::integer(), Reply} ----------- |
 * 
 *  Instruction = {run,   Cmd::string(), Options}   |
 *                {shell, Cmd::string(), Options}   |
 *                {list}                            | 
 *                {stop, OsPid::integer()}          |
 *                {kill, OsPid::integer(), Signal::integer()}
 *  Options = [Option]
 *  Option  = {cd, Dir::string()} | {env, [string()]} | {kill, Cmd::string()} |
 *            {user, User::string()} | {nice, Priority::integer()} |
 *            {stdout, Device::string()} | {stderr, Device::string()}
 *  Device  = null | stderr | stdout | File::string() | {append, File::string()}
 * 
 *  Reply = ok                      |       // For kill/stop commands
 *          {ok, OsPid}             |       // For run/shell command
 *          {ok, [OsPid]}           |       // For list command
 *          {error, Reason}         | 
 *          {exit_status, OsPid, Status}    // OsPid terminated with Status
 * 
 *  Reason = atom() | string()
 *  OsPid  = integer()
 *  Status = integer()
 * 
 **/

// Core includes
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

// Object includes
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <stdexcept>
#include <dirent.h>
#include <list>
#include <deque>
#include <sstream>
#include <setjmp.h>
#include <limits.h>
#include <pwd.h>

// System includes
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

// Our own includes
#include <ei.h>
#include "ei++.h"

#include "honeycomb_config.h"
#include "hc_support.h"
#include "babysitter.h"
#include "babysitter_daemon.h"

using namespace ei;
using namespace std;

//-------------------------------------------------------------------------
// Defines
//-------------------------------------------------------------------------

#define BUF_SIZE 2048
#define EXIT_FAILURE 1

/*---------------------------- Variables ------------------------------------*/

ei::Serializer eis(/* packet header size */ 2);

int alarm_max_time     = 12;
static bool superuser  = false;

MapChildrenT children;              // Map containing all managed processes started by this port program.
MapKillPidT  transient_pids;        // Map of pids of custom kill commands.

fd_set readfds;
struct sigaction sact, sterm;
int userid = 0;

// Configs
std::string config_file_dir;
extern FILE *yyin;
ConfigMapT   known_configs;         // Map containing all the known application configurations (in /etc/beehive/apps, unless otherwise specified)

/*---------------------------- Functions ------------------------------------*/

int   send_ok(int transId, pid_t pid = -1);
int   send_pid_status_term(const PidStatusT& stat);
int   send_error_str(int transId, bool asAtom, const char* fmt, ...);
int   send_pid_list(int transId, const MapChildrenT& children);

pid_t start_child(Honeycomb& op);
int   kill_child(pid_t pid, int sig, int transId, bool notify=true);
int   check_children(int& isTerminated, bool notify = true);
void  stop_child(pid_t pid, int transId, const TimeVal& now);
int   stop_child(Bee& bee, int transId, const TimeVal& now, bool notify = true);

void  setup_defaults();

void usage() {
  std::cerr << HELP_MESSAGE;
  exit(1);
}

/**
* Setup defaults for the program. These can be overriden after the
* command-line is parsed. This way we don't have variables we expect to be non-NULL 
* being NULL at runtime.
**/
void setup_defaults() {
  config_file_dir = "/etc/beehive/configs";
}

int parse_the_command_line(int argc, char* argv[]) {
  /**
  * Command line processing (TODO: Move this into a function)
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
        userid = pw->pw_uid;
        break;
    }
  }
  return 0;
}

//-------------------------------------------------------------------------
// MAIN
//-------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    setup_defaults();
    setup_signal_handlers();
    
    if (parse_the_command_line(argc, argv)) {
      return -1;
    }
    
    parse_config_dir(config_file_dir, known_configs); // Parse the config
    
    const int maxfd = eis.read_handle()+1;

    /**
     * Program loop. 
     * Daemonizing loop. This waits for signals
     * from the erlang process
     **/
    while (!terminated) {
      /* Detect "open" for serial pty slave */
      FD_ZERO (&readfds);
      FD_SET (eis.read_handle(), &readfds);

      sigsetjmp(jbuf, 1); oktojump = 0;
        
      // If there are children that are exiting, we can't do anything until they have exited
      // Check for pending signals arrived while we were in the signal handler
      check_pending();
      
      // If we are going to die, die
      if (terminated) break;

      oktojump = 1;
      ei::TimeVal timeout(5, 0);
      int cnt = select (maxfd, &readfds, (fd_set *)0, (fd_set *) 0, &timeout.timeval());
      int interrupted = (cnt < 0 && errno == EINTR);
      oktojump = 0;
        
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

        // Note that if we were using non-blocking reads, we'd also need to check for errno EWOULDBLOCK.
        if ((err = eis.read()) < 0) {
          terminated = 90-err;
          break;
        } 
                
        /* Our marshalling spec is that we are expecting a tuple {TransId, {Cmd::atom(), Arg1, Arg2, ...}} */
        if (eis.decodeTupleSize() != 2 || 
          (eis.decodeInt(transId)) < 0 || 
          (arity = eis.decodeTupleSize()) < 1)
        {
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
          case BUNDLE: {
            
            break;
          }
          case EXECUTE:
          case SHELL: {
            // {shell, Cmd::string(), Options::list()}
            Honeycomb comb;
            if (arity != 3 || comb.ei_decode(eis) < 0) {
              send_error_str(transId, false, comb.strerror());
              continue;
            }
            
            if (! comb.valid() ) {
              send_error_str(transId, false, "There was an error with the honeycomb. Not a valid bee: %s", strerror(errno));
            } else {
              pid_t pid;
              if ((pid = start_child(comb)) < 0)
                send_error_str(transId, false, "Couldn't start pid: %s", strerror(errno));
              else {
                Bee bee(comb, pid);
                children[pid] = bee;
                send_ok(transId, pid);
              }
            }
            break;
          }
          case STOP: {
            // {stop, OsPid::integer()}
            long pid;
            if (arity != 2 || (eis.decodeInt(pid)) < 0) {
              send_error_str(transId, true, "badarg");
              continue;
            }
              stop_child(pid, transId, TimeVal(TimeVal::NOW));
              break;
            }
          case KILL: {
            // {kill, OsPid::integer(), Signal::integer()}
            long pid, sig;
            if (arity != 3 || (eis.decodeInt(pid)) < 0 || (eis.decodeInt(sig)) < 0) {
              send_error_str(transId, true, "badarg");
              continue;
            } 
            if (superuser && children.find(pid) == children.end()) {
              send_error_str(transId, false, "Cannot kill a pid not managed by this application");
              continue;
            }
            kill_child(pid, sig, transId);
            break;
          }
          case LIST:
            // {list}
            if (arity != 1) {
            send_error_str(transId, true, "badarg");
            continue;
          }
          send_pid_list(transId, children);
          break;
        }
      }
    }

    sigsetjmp(jbuf, 1); 
    oktojump = 0;
    alarm(alarm_max_time);  // Die in <alarm_max_time> seconds if not done

    int old_terminated = terminated;
    terminated = 0;
    
    kill(0, SIGTERM); // Kill all children in our process group

    TimeVal now(TimeVal::NOW);
    TimeVal deadline(now, 6, 0);

    while (children.size() > 0) {
      sigsetjmp(jbuf, 1);
        
      while (exited_children.size() > 0 || signaled) {
        int term = 0;
        check_children(term, pipe_valid);
      }

      for(MapChildrenT::iterator it=children.begin(), end=children.end(); it != end; ++it)
        stop_child(it->first, 0, now);
    
      for(MapKillPidT::iterator it=transient_pids.begin(), end=transient_pids.end(); it != end; ++it) {
        kill(it->first, SIGKILL);
        transient_pids.erase(it);
      }
        
      if (children.size() == 0)
        break;
    
      TimeVal timeout(TimeVal::NOW);
      if (timeout < deadline) {
        timeout = deadline - timeout;
        
        oktojump = 1;
        select (0, (fd_set *)0, (fd_set *)0, (fd_set *) 0, &timeout);
        oktojump = 0;
      }
    }
    
    debug("Exiting (%d)\n", old_terminated);
    return old_terminated;
}

pid_t attempt_to_kill_child(const char* cmd, int user, int nice) {
  pid_t pid = fork();
  ei::StringBuffer<128> err;

  switch (pid) {
      case -1: 
        return -1;
      case 0: {
        // I am the child
        if ((user != INT_MAX && setegid(user) && setuid(user)) < 0) {
          err.write("Cannot set effective user to %d", user);
          perror(err.c_str());
          return EXIT_FAILURE;
        }
        const char* const argv[] = { getenv("SHELL"), "-c", cmd, (char*)NULL };
        if (execve((const char*)argv[0], (char* const*)argv, NULL) < 0) {
          perror(err.c_str());
          return EXIT_FAILURE;
        }
      }
      default:
        // I am the parent
        if (nice != INT_MAX && setpriority(PRIO_PROCESS, pid, nice) < 0) {
          err.write("Cannot set priority of pid %d to %d", pid, nice);
          perror(err.c_str());
        }
        return pid;
    }
}

/**
 * Implemented now to get it going mainly
 **/
pid_t start_child(Honeycomb& op) 
{
  ei::StringBuffer<128> err;
    
  const std::string base_dir = "/var/babysitter";
  //op.build_environment(base_dir, 040755);
  // return op.build_and_execute(base_dir, 040755);
  return 0;
}

int stop_child(Bee& bee, int transId, const TimeVal& now, bool notify) 
{
    bool use_kill = false;
    
    if (bee.kill_cmd_pid > 0 || bee.sigterm) {
        double diff = bee.deadline.diff(now);
        if (dbg)
            fprintf(stderr, "Deadline: %.3f\r\n", diff);
        // There was already an attempt to kill it.
        if (bee.sigterm && bee.deadline.diff(now) < 0) {
            // More than 5 secs elapsed since the last kill attempt
            kill(bee.cmd_pid, SIGKILL);
            kill(bee.kill_cmd_pid, SIGKILL);
            bee.sigkill = true;
        }
        if (notify) send_ok(transId);
        return 0;
    } else if (!bee.kill_cmd.empty()) {
        // This is the first attempt to kill this pid and kill command is provided.
        bee.kill_cmd_pid = attempt_to_kill_child(bee.kill_cmd.c_str(), INT_MAX, INT_MAX);
        if (bee.kill_cmd_pid > 0) {
            transient_pids[bee.kill_cmd_pid] = bee.cmd_pid;
            bee.deadline.set(now, 5);
            if (notify) send_ok(transId);
            return 0;
        } else {
            if (notify) send_error_str(transId, false, "bad kill command - using SIGTERM");
            use_kill = true;
            notify = false;
        }
    } else {
        // This is the first attempt to kill this pid and no kill command is provided.
        use_kill = true;
    }
    
    if (use_kill) {
        // Use SIGTERM/SIGKILL
        int n;
        if (!bee.sigterm && (n = kill_child(bee.cmd_pid, SIGTERM, transId, notify)) == 0) {
            bee.deadline.set(now, 5);
        } else if (!bee.sigkill && (n = kill_child(bee.cmd_pid, SIGKILL, 0, false)) == 0) {
            bee.deadline = now;
            bee.sigkill  = true;
        } else {
            n = 0; // FIXME
            // Failed to send SIGTERM & SIGKILL to the process - give up
            bee.sigkill = true;
            MapChildrenT::iterator it = children.find(bee.cmd_pid);
            if (it != children.end()) 
                children.erase(it);
        }
        bee.sigterm = true;
        return n;
    }
    return 0;
}

void stop_child(pid_t pid, int transId, const TimeVal& now)
{
  int n = 0;

  MapChildrenT::iterator it = children.find(pid);
  if (it == children.end()) {
    send_error_str(transId, false, "pid not alive");
    return;
  } else if ((n = kill(pid, 0)) < 0) {
    send_error_str(transId, false, "pid not alive (err: %d)", n);
    return;
  }
  stop_child(it->second, transId, now);
}

int kill_child(pid_t pid, int signal, int transId, bool notify)
{
  // We can't use -pid here to kill the whole process group, because our process is
  // the group leader.
  int err = kill(pid, signal);
  switch (err) {
    case 0:
      if (notify) send_ok(transId);
      break;
    case EINVAL:
      if (notify) send_error_str(transId, false, "Invalid signal: %d", signal);
      break;
    case ESRCH:
      if (notify) send_error_str(transId, true, "esrch");
      break;
    case EPERM:
      if (notify) send_error_str(transId, true, "eperm");
      break;
    default:
      if (notify) send_error_str(transId, true, strerror(err));
      break;
  }
  return err;
}

int check_children(int& isTerminated, bool notify)
{
  do {
        // For each process info in the <exited_children> queue deliver it to the Erlang VM
        // and removed it from the managed <children> map.
        std::deque< PidStatusT >::iterator it;
        while (!isTerminated && (it = exited_children.begin()) != exited_children.end()) {
            MapChildrenT::iterator i = children.find(it->first);
            MapKillPidT::iterator j;
            if (i != children.end()) {
                if (notify && send_pid_status_term(*it) < 0) {
                    isTerminated = 1;
                    return -1;
                }
                children.erase(i);
            } else if ((j = transient_pids.find(it->first)) != transient_pids.end()) {
                // the pid is one of the custom 'kill' commands started by us.
                transient_pids.erase(j);
            }
            
            exited_children.erase(it);
        }
        // Signaled flag indicates that there are more processes signaled SIGCHLD then
        // could be stored in the <exited_children> deque.
        if (signaled) {
            signaled = false;
            process_child_signal(-1);
        }
    } while (signaled && !isTerminated);
    
    TimeVal now(TimeVal::NOW);
    
    for (MapChildrenT::iterator it=children.begin(), end=children.end(); it != end; ++it) {
        int   status = ECHILD;
        pid_t pid = it->first;
        int n = kill(pid, 0);
        if (n == 0) { // process is alive
            if (it->second.kill_cmd_pid > 0 && now.diff(it->second.deadline) > 0) {
                kill(pid, SIGTERM);
                if ((n = kill(it->second.kill_cmd_pid, 0)) == 0)
                    kill(it->second.kill_cmd_pid, SIGKILL);
                it->second.deadline.set(now, 5, 0);
            }
            
            while ((n = waitpid(pid, &status, WNOHANG)) < 0 && errno == EINTR);
            if (n > 0)
                exited_children.push_back(std::make_pair(pid <= 0 ? n : pid, status));
            continue;
        } else if (n < 0 && errno == ESRCH) {
            if (notify) 
                send_pid_status_term(std::make_pair(it->first, status));
            children.erase(it);
        }
    }

    return 0;
}

int send_pid_list(int transId, const MapChildrenT& children)
{
  // Reply: {TransId, [OsPid::integer()]}
  eis.reset();
  eis.encodeTupleSize(2);
  eis.encode(transId);
  eis.encodeListSize(children.size());
  for(MapChildrenT::const_iterator it=children.begin(), end=children.end(); it != end; ++it)
      eis.encode(it->first);
  eis.encodeListEnd();
  return eis.write();
}

int send_error_str(int transId, bool asAtom, const char* fmt, ...)
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
  eis.encode(atom_t("error"));
  (asAtom) ? eis.encode(atom_t(str)) : eis.encode(str);
  return eis.write();
}

int send_ok(int transId, pid_t pid)
{
  eis.reset();
  eis.encodeTupleSize(2);
  eis.encode(transId);
  if (pid < 0)
    eis.encode(atom_t("ok"));
  else {
    eis.encodeTupleSize(2);
    eis.encode(atom_t("ok"));
    eis.encode(pid);
  }
  return eis.write();
}

int send_pid_status_term(const PidStatusT& stat)
{
  eis.reset();
  eis.encodeTupleSize(2);
  eis.encode(0);
  eis.encodeTupleSize(3);
  eis.encode(atom_t("exit_status"));
  eis.encode(stat.first);
  eis.encode(stat.second);
  return eis.write();
}
