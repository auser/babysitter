#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>             // For timeval struct
#include <time.h>                 // For time function
#include <string>
#include <map>
#include <deque>

#include "babysitter_utils.h"
#include "string_utils.h"
#include "time_utils.h"
#include "time_utils.h"
#include "printing_utils.h"

#include "command_info.h"
#include "process_manager.h"

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 4
#endif

// Globals
MapChildrenT                    children;
PidStatusDequeT                 exited_children;  // deque of processed SIGCHLD events
MapKillPidT                     transient_pids;   // Map of pids of custom kill commands.
int                             dbg = DEBUG_LEVEL;
struct sigaction                old_action;
struct sigaction                sact, sterm, spending;
bool                            signaled   = false;     // indicates that SIGCHLD was signaled
int                             terminated = 0;         // indicates that we got a SIGINT / SIGTERM event
int                             pending_sigalarm_signal = 0;
int                             run_as_user;
pid_t                           process_pid;

int process_child_signal(pid_t pid)
{
  if (exited_children.size() < exited_children.max_size()) {
    int status;
    pid_t ret;
    while ((ret = waitpid(pid, &status, WNOHANG)) < 0 && errno == EINTR);

    if (ret < 0 && errno == ECHILD) {
      int status = ECHILD;
      if (kill(pid, 0) == 0) // process likely forked and is alive
        status = 0;
      if (status != 0)
        exited_children.push_back(std::make_pair(pid <= 0 ? ret : pid, status));
    } else if (pid <= 0)
      exited_children.push_back(std::make_pair(ret, status));
    else if (ret == pid)
      exited_children.push_back(std::make_pair(pid, status));
    else
      return -1;
    return 1;
  } else {
    // else - defer calling waitpid() for later
    signaled = true;
    return 0;
  }
}   

void pending_signals(int sig) 
{
  pending_sigalarm_signal = 1;
  alarm(0);
}

void gotsignal(int signal)
{
  if (signal == SIGTERM || signal == SIGINT || signal == SIGPIPE)
    terminated = 1;
}

void gotsigchild(int signal, siginfo_t* si, void* context)
{
    // If someone used kill() to send SIGCHLD ignore the event
    if (signal != SIGCHLD) return;

    process_child_signal(si->si_pid);
}

pid_t start_child(int command_argc, const char** command_argv, const char *cd, const char** env, int user, int nice)
{
  pid_t pid = fork();

  switch (pid) {
  case -1: 
    return -1;
  case 0: {
    // In child process
    // if (user != INT_MAX && setresuid(user, user, user) < 0) {
    //   fperror("Cannot set effective user to %d", user);
    //   return EXIT_FAILURE;
    // }
    setup_signal_handlers();
    // const char* const argv[] = { getenv("SHELL"), "-c", cmd, (char*)NULL };
    if (cd != NULL && cd[0] != '\0' && chdir(cd) < 0) {
      fperror("Cannot chdir to '%s'", cd);
      return EXIT_FAILURE;
    }
    
    printf("running command: %s\n", command_argv[0]);
    for (int i = 0; i < command_argc; i++)
      printf("command_argv[%d] = %s\n", i, command_argv[i]);
    
    if (execve((const char*)command_argv[0], (char* const*)command_argv, (char* const*) env) < 0) {
      fperror("Cannot execute %s: %s\n", command_argv[0], strerror(errno));
      exit(-1);
    }
    printf("Child exited!\n");
    exit(0);
  }
  default:
    // In parent process
    if (nice != INT_MAX && setpriority(PRIO_PROCESS, pid, nice) < 0) {
      fperror("Cannot set priority of pid %d to %d", pid, nice);
    }
    printf("pid: %d\n", pid);
    return pid;
  }
}

int stop_child(CmdInfo& ci, int transId, time_t &now, bool notify)
{
  bool use_kill = false;
  tm * ptm;
  
  if (ci.kill_cmd_pid() > 0 || ci.sigterm()) {
    double diff = timediff(ci.deadline(), now);
      // There was already an attempt to kill it.
    if (ci.sigterm() && diff < 0) {
      // More than 5 secs elapsed since the last kill attempt
      kill(ci.cmd_pid(), SIGKILL);
      kill(ci.kill_cmd_pid(), SIGKILL);
      ci.set_sigkill(true);
    }
    if (notify) send_ok(transId);
    return 0;
  } else if (strncmp(ci.kill_cmd(), "", 1) != 0) {
   // This is the first attempt to kill this pid and kill command is provided.
   char **command_argv = {0};
   int command_argc = 0;
   if ((command_argc = argify(ci.kill_cmd(), &command_argv)) < 1) {
     fperror("Uh oh!\n");
     return 0;
   }
   ci.set_kill_cmd_pid(start_child(command_argc, (const char**)command_argv, NULL, NULL, INT_MAX, INT_MAX));
   if (ci.kill_cmd_pid() > 0) {
     transient_pids[ci.kill_cmd_pid()] = ci.cmd_pid();
     time_t ci_time = ci.deadline();
     ptm = gmtime( &ci_time  );
     ptm->tm_sec += 1;
     ci.set_deadline(mktime(ptm));
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
   // Use SIGTERM / SIGKILL to nuke the pid
   int n;
   if (!ci.sigterm() && (n = kill_child(ci.cmd_pid(), SIGTERM, transId, notify)) == 0) {
     time_t ci_time = ci.deadline();
     ptm = gmtime( &ci_time  );
     ptm->tm_sec += 1;
     ci.set_deadline(mktime(ptm));
   } else if (!ci.sigkill() && (n = kill_child(ci.cmd_pid(), SIGKILL, 0, false)) == 0) {
     ci.set_deadline(now);
     ci.set_sigkill(true);
   } else {
     n = 0; // FIXME
     // Failed to send SIGTERM & SIGKILL to the process - give up
     ci.set_sigkill(true);
     MapChildrenT::iterator it = children.find(ci.cmd_pid());
     if (it != children.end()) 
       children.erase(it);
     }
     ci.set_sigterm(true);
     return n;
 }
 return 0;
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


void stop_child(pid_t pid, int transId, time_t &now)
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
  // stop_child(CmdInfo& ci, int transId, time_t &now, bool notify)
  stop_child(it->second, (int)transId, now);
}


void terminate_all()
{  
  time_t now_seconds, timeout_seconds, deadline;
  now_seconds = time (NULL);
  
  while (children.size() > 0) {
    while (exited_children.size() > 0 || signaled) {
      int term = 0;
      check_children(term);
    }
    
    // Attempt to kill the children
    for(MapChildrenT::iterator it=children.begin(), end=children.end(); it != end; ++it) {
      stop_child((pid_t)it->first, 0, now_seconds);
    }
    
    // Definitely kill the transient_pids
    for(MapKillPidT::iterator it=transient_pids.begin(), end=transient_pids.end(); it != end; ++it) {
      kill(it->first, SIGKILL);
      transient_pids.erase(it);
    }
      
    if (children.size() == 0) break;
    
    // give it a deadline to kill
    timeout_seconds = time (NULL);
    struct tm* ptm = gmtime ( &timeout_seconds );
    ptm->tm_sec += 1;
    deadline = mktime(ptm);
    
    if (timeout_seconds < deadline) {
      timeout_seconds = deadline - timeout_seconds;
      struct timeval tv;
      tv.tv_sec = 5;
      tv.tv_usec = 0;
      select (0, (fd_set *)0, (fd_set *)0, (fd_set *) 0, &tv);
    }
  }
}

void setup_signal_handlers()
{
  sterm.sa_handler = gotsignal;
  sigemptyset(&sterm.sa_mask);
  sigaddset(&sterm.sa_mask, SIGCHLD);
  sterm.sa_flags = 0;
  sigaction(SIGINT,  &sterm, NULL);
  sigaction(SIGTERM, &sterm, NULL);
  sigaction(SIGHUP,  &sterm, NULL);
  sigaction(SIGPIPE, &sterm, NULL);
  
  sact.sa_handler = NULL;
  sact.sa_sigaction = gotsigchild;
  sigemptyset(&sact.sa_mask);
  sact.sa_flags = SA_SIGINFO | SA_RESTART | SA_NOCLDSTOP | SA_NODEFER;
  sigaction(SIGCHLD, &sact, NULL);
  
  spending.sa_handler = pending_signals;
  sigemptyset(&spending.sa_mask);
  spending.sa_flags = 0;
  sigaction(SIGALRM, &spending, NULL);
}

void check_pending()
{
  sigset_t  set;
  int info;
  int sig;
  struct itimerval tval;
  struct timeval interval = {0, 20000};
  sigemptyset(&set);
  if (sigpending(&set) == 0) {
    pending_sigalarm_signal = 0;
    tval.it_interval = interval;
    tval.it_value = interval;
    setitimer(ITIMER_REAL, &tval, NULL);
    while (((sig = sigwait(&set, &info)) > 0 || errno == EINTR) && !pending_sigalarm_signal )
    switch (sig) {
      case SIGCHLD:   gotsignal(sig); break;
      case SIGTERM:
      case SIGINT:
      case SIGHUP:    gotsignal(sig); break;
      case SIGALRM:   printf("got SIGALRM\n"); break;
      default:        break;
    }
  }
  // Clear
  pending_sigalarm_signal = 0;
}

int check_children(int& isTerminated)
{
  do {
    std::deque<PidStatusT>::iterator it;
    while (!isTerminated && (it = exited_children.begin()) != exited_children.end()) {
      MapChildrenT::iterator i = children.find(it->first);
      MapKillPidT::iterator j;
      if (i != children.end()) {
        // if (notify && send_pid_status_term(*it) < 0) {
        //   isTerminated = 1;
        //   return -1;
        // }
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
    
  time_t now = time (NULL);
  
  for (MapChildrenT::iterator it=children.begin(), end=children.end(); it != end; ++it) {
    int   status = ECHILD;
    pid_t pid = it->first;
    int n = kill(pid, 0);
    if (n == 0) { // process is alive
      if (it->second.kill_cmd_pid() > 0 && timediff(now, it->second.deadline()) > 0) {
        kill(pid, SIGTERM);
        if ((n = kill(it->second.kill_cmd_pid(), 0)) == 0) kill(it->second.kill_cmd_pid(), SIGKILL);
        
        time_t ci_time = it->second.deadline();
        struct tm* ptm = gmtime( &ci_time  );
        ptm->tm_sec += 1;
        it->second.set_deadline(mktime(ptm));
      }
            
      while ((n = waitpid(pid, &status, WNOHANG)) < 0 && errno == EINTR);
      if (n > 0) exited_children.push_back(std::make_pair(pid <= 0 ? n : pid, status));
      continue;
    } else if (n < 0 && errno == ESRCH) {
      // if (notify) send_pid_status_term(std::make_pair(it->first, status));
      children.erase(it);
    }
  }

  return 0;
}

void setup_defaults()
{	
  run_as_user = getuid();
  process_pid = (int)getpid();
  setup_signal_handlers();
}