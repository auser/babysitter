#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>             // For timeval struct
#include <sys/wait.h>             // For waitpid
#include <time.h>                 // For time function
#include <string>
#include <map>
#include <deque>
#include <setjmp.h>

#include "babysitter_utils.h"
#include "babysitter_types.h"
#include "string_utils.h"
#include "time_utils.h"
#include "print_utils.h"
#include "callbacks.h"

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
int                             run_as_user;
pid_t                           process_pid;
sigjmp_buf                      saved_jump_buf;
int                             pm_can_jump = 0;
extern int                      dbg;

/*
* Set the loop ready for jumping on signal reception, if necessary
*/
int pm_next_loop()
{
  sigsetjmp(saved_jump_buf, 1); pm_can_jump = 0;
  
  while (!terminated && (exited_children.size() > 0 || signaled)) pm_check_children(terminated);
  pm_check_pending_processes(); // Check for pending signals arrived while we were in the signal handler
  debug(dbg, 4, "terminated in pm_check_pending_processes... %d\n", (int)terminated);
  
  pm_can_jump = 1;
  if (terminated) return -1;
  else return 0;
}

int process_child_signal(pid_t pid)
{
  if (exited_children.size() < SIGCHLD_MAX_SIZE) { // exited_children.max_size() ??
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

void pm_reload(void *data)
{
  // Nothing yet
}

void pm_gotsignal(int signal)
{
  debug(dbg, 4, "Got signal: %d\n", signal);
  
  switch(signal) {
    case SIGHUP:
      run_callbacks("pm_reload", &signal);
      break;
    case SIGTERM:
    case SIGINT:
      terminated = 1;
    default:
      if (pm_can_jump) siglongjmp(saved_jump_buf, 1);
    break;
  }
}

void pm_gotsigchild(int signal, siginfo_t* si, void* context)
{
  // If someone used kill() to send SIGCHLD ignore the event
  if (signal != SIGCHLD) return;
    
  debug(dbg, 4, "Got SIGCHLD: %d for %d\n", signal, (int)si->si_pid);
  process_child_signal(si->si_pid);
    
  if (pm_can_jump) siglongjmp(saved_jump_buf, 1);
}

int setup_pm_pending_alarm()
{
  struct itimerval tval;
  struct timeval interval = {0, 20000};
  
  tval.it_interval = interval;
  tval.it_value = interval;
  setitimer(ITIMER_REAL, &tval, NULL);
  
  struct sigaction spending;
  spending.sa_handler = pm_gotsignal;
  sigemptyset(&spending.sa_mask);
  spending.sa_flags = 0;
  sigaction(SIGALRM, &spending, NULL);
  
  return 0;
}

pid_t pm_start_child(const char* command_argv, const char *cd, const char** env, int user, int nice)
{
  pid_t pid = fork();
  switch (pid) {
  case -1: 
    return -1;
  case 0: {
    setup_signal_handlers();
    const char* const argv[] = { getenv("SHELL"), "-c", command_argv, (char*)NULL };
    if (cd != NULL && cd[0] != '\0' && chdir(cd) < 0) {
      fperror("Cannot chdir to '%s'", cd);
      return EXIT_FAILURE;
    }
    
    if (execve((const char*)argv[0], (char* const*)argv, (char* const*) env) < 0) {
      fperror("Cannot execute %s: %s\n", argv[0], strerror(errno));
      exit(-1);
    }
    debug(dbg, 1, "There was an error in execve\n");
    exit(-1);
  }
  default:
    // In parent process
    if (nice != INT_MAX && setpriority(PRIO_PROCESS, pid, nice) < 0) {
      fperror("Cannot set priority of pid %d to %d", pid, nice);
    }
    return pid;
  }
}

int pm_stop_child(CmdInfo& ci, int transId, time_t &now, bool notify)
{
  debug(dbg, 1, "Called pm_stop_child\n");
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
    if (notify) handle_ok(transId);
    return 0;
  } else if (strncmp(ci.kill_cmd(), "", 1) != 0) {
   // This is the first attempt to kill this pid and kill command is provided.
   ci.set_kill_cmd_pid(pm_start_child((const char*)ci.kill_cmd(), NULL, NULL, INT_MAX, INT_MAX));
   if (ci.kill_cmd_pid() > 0) {
     transient_pids[ci.kill_cmd_pid()] = ci.cmd_pid();
     time_t ci_time = ci.deadline();
     ptm = gmtime( &ci_time  );
     ptm->tm_sec += 1;
     ci.set_deadline(mktime(ptm));
     if (notify) handle_ok(transId);
     return 0;
   } else {
     if (notify) handle_error_str(transId, false, "bad kill command - using SIGTERM");
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
   if (!ci.sigterm() && (n = pm_kill_child(ci.cmd_pid(), SIGTERM, transId, notify)) == 0) {
     time_t ci_time = ci.deadline();
     ptm = gmtime( &ci_time  );
     ptm->tm_sec += 1;
     ci.set_deadline(mktime(ptm));
   } else if (!ci.sigkill() && (n = pm_kill_child(ci.cmd_pid(), SIGKILL, 0, false)) == 0) {
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

int pm_kill_child(pid_t pid, int signal, int transId, bool notify)
{
  // We can't use -pid here to kill the whole process group, because our process is
  // the group leader.
  debug(dbg, 2, "CAlling pm_kill_child on pid: %d\n", (int)pid);
  int err = kill(pid, signal);
  switch (err) {
    case 0:
      if (notify) handle_ok(transId);
      break;
    case EINVAL:
      if (notify) handle_error_str(transId, false, "Invalid signal: %d", signal);
      break;
    case ESRCH:
      if (notify) handle_error_str(transId, true, "esrch");
      break;
    case EPERM:
      if (notify) handle_error_str(transId, true, "eperm");
      break;
    default:
      if (notify) handle_error_str(transId, true, strerror(err));
      break;
  }
  return err;
}


void pm_stop_child(pid_t pid, int transId, time_t &now)
{
  int n = 0;

  MapChildrenT::iterator it = children.find(pid);
  if (it == children.end()) {
    handle_error_str(transId, false, "pid not alive");
    return;
  } else if ((n = kill(pid, 0)) < 0) {
    handle_error_str(transId, false, "pid not alive (err: %d)", n);
    return;
  }
  // pm_stop_child(CmdInfo& ci, int transId, time_t &now, bool notify)
  pm_stop_child(it->second, (int)transId, now);
}


void pm_terminate_all()
{  
  time_t now_seconds, timeout_seconds, deadline;
  now_seconds = time (NULL);
  
  while (children.size() > 0) {
    while (exited_children.size() > 0 || signaled) {
      int term = 0;
      pm_check_children(term);
    }
    
    // Attempt to kill the children
    for(MapChildrenT::iterator it=children.begin(), end=children.end(); it != end; ++it) {
      pm_stop_child((pid_t)it->first, 0, now_seconds);
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
  sterm.sa_handler = pm_gotsignal;
  sigemptyset(&sterm.sa_mask);
  sigaddset(&sterm.sa_mask, SIGCHLD);
  sterm.sa_flags = 0;
  sigaction(SIGINT,  &sterm, NULL);
  sigaction(SIGTERM, &sterm, NULL);
  sigaction(SIGHUP,  &sterm, NULL);
  sigaction(SIGPIPE, &sterm, NULL);
  
  sact.sa_handler = NULL;
  sact.sa_sigaction = pm_gotsigchild;
  sigemptyset(&sact.sa_mask);
  sact.sa_flags = SA_SIGINFO | SA_RESTART | SA_NOCLDSTOP | SA_NODEFER;
  sigaction(SIGCHLD, &sact, NULL);
}

int pm_check_pending_processes()
{
  int sig = 0;
  sigset_t sigset;
  setup_pm_pending_alarm();
  if ((sigemptyset(&sigset) == -1)
      || (sigaddset(&sigset, SIGALRM) == -1)
      || (sigaddset(&sigset, SIGINT) == -1)
      || (sigaddset(&sigset, SIGTERM) == -1)
      || (sigprocmask( SIG_BLOCK, &sigset, NULL) == -1)
     )
    fperror("Failed to block signals before sigwait\n");
  
  if (sigpending(&sigset) == 0) {
    while (errno == EINTR)
      if (sigwait(&sigset, &sig) == -1) {
        fperror("sigwait");
        return -1;
      }
    debug(dbg, 4, "%d received signal: %d\n", (int)getpid(), sig);
    pm_gotsignal(sig);
  }
  return 0;
}

int pm_check_children(int& isTerminated)
{
  debug(dbg, 4, "pm_check_children isTerminated: %d and signaled: %d\n", (int)isTerminated, signaled);
  do {
    std::deque<PidStatusT>::iterator it;
    while (!isTerminated && (it = exited_children.begin()) != exited_children.end()) {
      MapChildrenT::iterator i = children.find(it->first);
      MapKillPidT::iterator j;
      if (i != children.end()) {
        if (handle_pid_status_term(*it) < 0) {
          isTerminated = 1;
          return -1;
        }
        children.erase(i);
      } else if ((j = transient_pids.find(it->first)) != transient_pids.end()) {
        // the pid is one of the custom 'kill' commands started by babysitter.
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
  
  debug(dbg, 4, "Iterating through the children (%d)...\n", children.size());
  for (MapChildrenT::iterator it=children.begin(), end=children.end(); it != end; ++it) {
    int   status = ECHILD;
    pid_t pid = it->first;
    debug(dbg, 4, "Checking on pid: %d\n", (int)pid);
    int n = kill(pid, 0);
    if (n == 0) { // process is alive
      if (it->second.kill_cmd_pid() > 0 && timediff(now, it->second.deadline()) > 0) {
        debug(dbg, 2, "Sending SIGTERM to pid: %d\n", pid);
        kill(pid, SIGTERM);
        if ((n = kill(it->second.kill_cmd_pid(), 0)) == 0) kill(it->second.kill_cmd_pid(), SIGKILL);
        
        time_t ci_time = it->second.deadline();
        struct tm* ptm = gmtime( &ci_time  );
        ptm->tm_sec += 1;
        it->second.set_deadline(mktime(ptm));
      }
      
      debug(dbg, 4, "Waiting for pid: %d as WNOHANG\n", pid);
      while ((n = waitpid(pid, &status, WNOHANG)) < 0 && errno == EINTR) ;
      if (n > 0) exited_children.push_back(std::make_pair(pid <= 0 ? n : pid, status));
      continue;
    } else if (n < 0 && errno == ESRCH) {
      handle_pid_status_term(std::make_pair(it->first, status));
      children.erase(it);
    }
  }
  debug(dbg, 4, "Done iterating through the children\n");
  return 0;
}

void pm_setup()
{
  debug(dbg, 1, "Setting up process manager defaults\n");
  run_as_user = getuid();
  process_pid = (int)getpid();
  exited_children.resize(SIGCHLD_MAX_SIZE);
  setup_signal_handlers();
  debug(dbg, 4, "exited_children (%d) max_size: %d\n", (int)exited_children.size(), (int)exited_children.max_size());
  // Callbacks
  register_callback("pm_reload", pm_reload);
}
