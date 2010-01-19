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
#include "babysitter.h"
#include <ei.h>
#include "ei++.h"

using namespace ei;
using namespace std;

//-------------------------------------------------------------------------
// Defines
//-------------------------------------------------------------------------

#define BUF_SIZE 2048
#define EXIT_FAILURE 1

//-------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------
typedef set<string> string_set;

class CmdInfo;

typedef unsigned char byte;
typedef int   exit_status_t;
typedef pid_t kill_cmd_pid_t;
typedef std::pair<pid_t, exit_status_t>     PidStatusT;
typedef std::pair<pid_t, CmdInfo>           PidInfoT;
typedef std::map <pid_t, CmdInfo>           MapChildrenT;
typedef std::pair<kill_cmd_pid_t, pid_t>    KillPidStatusT;
typedef std::map <kill_cmd_pid_t, pid_t>    MapKillPidT;

class CmdOptions {
    ei::StringBuffer<256>   m_tmp;       // Temporary storage
    std::stringstream       m_err;       // Error message to use to pass backwards to the erlang caller
    std::string             m_cmd;       // The command to execute
    std::string             m_cd;        // The directory to execute the command (generated, if not given)
    std::string             m_stdout;    // The stdout to use for the execution of the command
    std::string             m_stderr;    // The stderr to use for the execution of the command
    std::string             m_kill_cmd;  // A special command to kill the process (if needed)
    std::string             m_image;     // The path to the image file to mount
    std::list<std::string>  m_env;       // A list of environment variables to use when starting
    long                    m_nice;      // The "niceness" level
    size_t                  m_size;      // The heap/stack size
    int                     m_user;      // run as user (generated if not given)
    const char**            m_cenv;      // The string list of environment variables

public:

    CmdOptions() : m_tmp(0, 256), m_image(""), m_nice(INT_MAX), m_size(0), m_user(INT_MAX), m_cenv(NULL) {}
    ~CmdOptions() { delete [] m_cenv; m_cenv = NULL; }

    const char*  strerror() const { return m_err.str().c_str(); }
    const char*  cmd()      const { return m_cmd.c_str(); }
    const char*  cd()       const { return m_cd.c_str(); }
    const char*  image()    const { return m_image.c_str(); }
    char* const* env()      const { return (char* const*)m_cenv; }
    const char*  kill_cmd() const { return m_kill_cmd.c_str(); }
    int          user()     const { return m_user; }
    int          nice()     const { return m_nice; }
    
    int ei_decode(ei::Serializer& ei);
};

/// Contains run-time info of a child OS process.
/// When a user provides a custom command to kill a process this 
/// structure will contain its run-time information.
struct CmdInfo {
    std::string     cmd;            // Executed command
    std::string     mount_point;    // Mount-point related to this process
    pid_t           cmd_pid;        // Pid of the custom kill command
    std::string     kill_cmd;       // Kill command to use (if provided - otherwise use SIGTERM)
    kill_cmd_pid_t  kill_cmd_pid;   // Pid of the command that <pid> is supposed to kill
    ei::TimeVal     deadline;       // Time when the <cmd_pid> is supposed to be killed using SIGTERM.
    bool            sigterm;        // <true> if sigterm was issued.
    bool            sigkill;        // <true> if sigkill was issued.

    CmdInfo() : cmd_pid(-1), kill_cmd_pid(-1), sigterm(false), sigkill(false) {}
    CmdInfo(const CmdInfo& ci) {
        new (this) CmdInfo(ci.cmd.c_str(), ci.kill_cmd.c_str(), ci.cmd_pid);
    }
    CmdInfo(const char* _cmd, const char* _kill_cmd, pid_t _cmd_pid) {
        new (this) CmdInfo();
        cmd      = _cmd;
        cmd_pid  = _cmd_pid;
        kill_cmd = _kill_cmd;
    }
    CmdInfo(const char* _cmd, const char* _mount_point, const char* _kill_cmd, pid_t _cmd_pid) {
        new (this) CmdInfo();
        mount_point = _mount_point;
        cmd         = _cmd;
        cmd_pid     = _cmd_pid;
        kill_cmd    = _kill_cmd;
    }
};


//-------------------------------------------------------------------------
// Global variables
//-------------------------------------------------------------------------

ei::Serializer eis(/* packet header size */ 2);

sigjmp_buf  jbuf;
int alarm_max_time     = 12;
static bool debug      = false;
static bool oktojump   = false;
static bool signaled   = false;     // indicates that SIGCHLD was signaled
static int  terminated = 0;         // indicates that we got a SIGINT / SIGTERM event
static bool superuser  = false;
static bool pipe_valid = true;

MapChildrenT children;              // Map containing all managed processes started by this port program.
MapKillPidT  transient_pids;        // Map of pids of custom kill commands.

#define SIGCHLD_MAX_SIZE 4096
std::deque< PidStatusT > exited_children;  // deque of processed SIGCHLD events

//-------------------------------------------------------------------------
// Local Functions
//-------------------------------------------------------------------------

int   send_ok(int transId, pid_t pid = -1);
int   send_pid_status_term(const PidStatusT& stat);
int   send_error_str(int transId, bool asAtom, const char* fmt, ...);
int   send_pid_list(int transId, const MapChildrenT& children);

pid_t start_child(const CmdOptions& op);
int   kill_child(pid_t pid, int sig, int transId, bool notify=true);
int   check_children(int& isTerminated, bool notify = true);
void  stop_child(pid_t pid, int transId, const TimeVal& now);
int   stop_child(CmdInfo& ci, int transId, const TimeVal& now, bool notify = true);

/**
 * We received a signal for the child pid process
 * Wait for the pid to exit if it hasn't only if the and it's an interrupt process
 * make sure the process get the pid and signal to it
 **/
int process_child_signal(pid_t pid)
{
  // If we have less exited_children then allowed
  if (exited_children.size() < exited_children.max_size()) {
    int status;
    pid_t ret;
    
    while ((ret = waitpid(pid, &status, WNOHANG)) < 0 && errno == EINTR);
    
    // Check for the return on the child process
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

// int send_error_str(int transId, bool asAtom, const char* fmt, ...)
void dmesg(const char *fmt, ...) {
  if(debug)
  {
    char str[BUF_SIZE];
    va_list vargs;
    va_start (vargs, fmt);
    vsnprintf(str, sizeof(str), fmt, vargs);
    va_end   (vargs);

    fprintf(stderr, fmt, str);
  }
}

const char *DEV_RANDOM = "/dev/urandom";
static uid_t random_uid() {
  uid_t u;
  for (unsigned char i = 0; i < 10; i++) {
    int rndm = open(DEV_RANDOM, O_RDONLY);
    if (sizeof(u) != read(rndm, reinterpret_cast<char *>(&u), sizeof(u))) {
      continue;
    }
    close(rndm);

    if (u > 0xFFFF) {
      return u;
    }
  }

  printf("Could not generate a good random UID after 10 attempts. Bummer!");
  exit(-1);
}


// We've received a signal to process
void gotsignal(int sig) {
  if (debug) dmesg("Got signal: %d\n", sig);
  if (oktojump) siglongjmp(jbuf, 1);
  switch (sig) {
    case SIGTERM:
    case SIGINT:
      terminated = 1;
      break;
    case SIGPIPE:
      terminated = 1;
      pipe_valid = false;
      break;
  }
}

/**
 * Got a signal from a child
 **/
void gotsigchild(int signal, siginfo_t* si, void* context) {
  // If someone used kill() to send SIGCHLD ignore the event
  if (si->si_code == SI_USER || signal != SIGCHLD) return;
  
  dmesg("Process %d exited (sig=%d)\r\n", si->si_pid, signal);
  process_child_signal(si->si_pid);
  if (oktojump) siglongjmp(jbuf, 1);
}

/**
 * Check for the pending messages, so we don't miss any
 * child signal messages
 **/
void check_pending() {
  static const struct timespec timeout = {0, 0};
  int sig;
  sigset_t  set;
  siginfo_t info;
  sigemptyset(&set);
  if (sigpending(&set) == 0) {
    while ((sig = sigtimedwait(&set, &info, &timeout)) > 0 || errno == EINTR)
    switch (sig) {
      case SIGCHLD:   gotsigchild(sig, &info, NULL); break;
      case SIGPIPE:   pipe_valid = false; /* intentionally follow through */
      case SIGTERM:
      case SIGINT:
      case SIGHUP:    gotsignal(sig); break;
      default:        break;
    }
  }
}

void usage() {
  std::cerr << HELP_MESSAGE;
  exit(1);
}

//-------------------------------------------------------------------------
// MAIN
//-------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    fd_set readfds;
    struct sigaction sact, sterm;
    int userid = 0;
    
    // Deque of all pids that exited and have their exit status available.
    exited_children.resize(SIGCHLD_MAX_SIZE);
    
    /**
     * Setup the signal handlers for *this* process
     **/
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
    
    /**
     * Command line processing
     **/
    char c;
    while (-1 != (c = getopt(argc, argv, "a:Dhn"))) {
      switch (c) {
        case 'h':
        case 'H':
         usage();
         return 1;
        case 'D':
          debug = true;
          eis.debug(true);
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

    /** 
     * If we are root, switch to non-root user and set capabilities
     * to be able to adjust niceness and run commands as other users.
     * Currently, you MUST be root to run babysitter because root
     * is the only user that has access to mounting loop-back devices
     **/
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
      while (!terminated && (exited_children.size() > 0 || signaled)) check_children(terminated);
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
        enum CmdTypeT { 
          EXECUTE,
          SHELL,
          STOP,
          KILL,
          LIST
        } cmd;
        const char* cmds[] = {
          "run",
          "shell",
          "stop",
          "kill",
          "list"
        };

        // Determine which of the commands was called
        if ((int)(cmd = (CmdTypeT) eis.decodeAtomIndex(cmds, command)) < 0) {
          if (send_error_str(transId, false, "Unknown command: %s", command.c_str()) < 0) {
            terminated = 11; 
            break;
          } else continue;
        }

        switch (cmd) {
          case EXECUTE:
          case SHELL: {
            // {shell, Cmd::string(), Options::list()}
            CmdOptions po;
            if (arity != 3 || po.ei_decode(eis) < 0) {
              send_error_str(transId, false, po.strerror());
              continue;
            }

            pid_t pid;
            if ((pid = start_child(po)) < 0)
              send_error_str(transId, false, "Couldn't start pid: %s", strerror(errno));
            else {
              CmdInfo ci(po.cmd(), po.kill_cmd(), pid);
              children[pid] = ci;
              send_ok(transId, pid);
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
    
    dmesg("Exiting (%d)\n", old_terminated);
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

pid_t start_child(const CmdOptions& op) 
{
  ei::StringBuffer<128> err;
    
  printf("Building chroot path!\n\n");
  /**
   * Create the chroot, thank you
   **/  
  
  pid_t pid = fork();
  
  switch (pid) {
    case -1: 
      err.write("Forking fail\n");
      return -1;
    case 0: {
      // I am the child
      // This will exit on failture
      printf("\n------->>>> in build_and_execute_chroot <<<<-----------\n\n");

      /**
       * Setup the user and drop into effective user first
       **/
      uid_t real_user; // Save these users
      if (op.user()) 
        real_user = op.user();
      else 
        real_user = random_uid();

      printf("--> The real user: %d\n", real_user);

      // Set the process group IDS first
      if (setresgid(-1, real_user, getegid()) || setresuid(-1, real_user, geteuid()) || getegid() != real_user || geteuid() != real_user ) {   
        fprintf(stderr,"\nSetting Group ID %u\n",real_user);
        perror("setresgid");
        exit(-1);
      }

      const char* const argv[] = { getenv("SHELL"), "-c", op.cmd(), (char*)NULL };
      if (op.cd() != NULL && op.cd()[0] != '\0' && chdir(op.cd()) < 0) {
        err.write("Cannot chdir to '%s'", op.cd());
        perror(err.c_str());
        return EXIT_FAILURE;
      }
      if (execve((const char*)argv[0], (char* const*)argv, op.env()) < 0) {
        err.write("Cannot execute '%s'", op.cd());
        perror(err.c_str());
        return EXIT_FAILURE;
      }
      return 0;
    }
    default:
      // I am the parent
      if (op.nice() != INT_MAX && setpriority(PRIO_PROCESS, pid, op.nice()) < 0) {
        err.write("Cannot set priority of pid %d to %d", pid, op.nice());
        perror(err.c_str());
      }
      return pid;
  }
}

int stop_child(CmdInfo& ci, int transId, const TimeVal& now, bool notify) 
{
    bool use_kill = false;
    
    if (ci.kill_cmd_pid > 0 || ci.sigterm) {
        double diff = ci.deadline.diff(now);
        if (debug)
            fprintf(stderr, "Deadline: %.3f\r\n", diff);
        // There was already an attempt to kill it.
        if (ci.sigterm && ci.deadline.diff(now) < 0) {
            // More than 5 secs elapsed since the last kill attempt
            kill(ci.cmd_pid, SIGKILL);
            kill(ci.kill_cmd_pid, SIGKILL);
            ci.sigkill = true;
        }
        if (notify) send_ok(transId);
        return 0;
    } else if (!ci.kill_cmd.empty()) {
        // This is the first attempt to kill this pid and kill command is provided.
        ci.kill_cmd_pid = attempt_to_kill_child(ci.kill_cmd.c_str(), INT_MAX, INT_MAX);
        if (ci.kill_cmd_pid > 0) {
            transient_pids[ci.kill_cmd_pid] = ci.cmd_pid;
            ci.deadline.set(now, 5);
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
        if (!ci.sigterm && (n = kill_child(ci.cmd_pid, SIGTERM, transId, notify)) == 0) {
            ci.deadline.set(now, 5);
        } else if (!ci.sigkill && (n = kill_child(ci.cmd_pid, SIGKILL, 0, false)) == 0) {
            ci.deadline = now;
            ci.sigkill  = true;
        } else {
            n = 0; // FIXME
            // Failed to send SIGTERM & SIGKILL to the process - give up
            ci.sigkill = true;
            MapChildrenT::iterator it = children.find(ci.cmd_pid);
            if (it != children.end()) 
                children.erase(it);
        }
        ci.sigterm = true;
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

int CmdOptions::ei_decode(ei::Serializer& ei)
{
    // {Cmd::string(), [Option]}
    //      Option = {env, Strings} | {cd, Dir} | {kill, Cmd}
    int sz;
    std::string op, val;
    
    m_err.str("");
    delete [] m_cenv;
    m_cenv = NULL;
    m_env.clear();
    m_nice = INT_MAX;
    
    if (eis.decodeString(m_cmd) < 0) {
        m_err << "badarg: cmd string expected or string size too large";
        return -1;
    } else if ((sz = eis.decodeListSize()) < 0) {
        m_err << "option list expected";
        return -1;
    } else if (sz == 0) {
        m_cd  = "";
        m_kill_cmd = "";
        return 0;
    }

    for(int i=0; i < sz; i++) {
        enum OptionT            { CD,   ENV,   KILL,   NICE,   USER,   STDOUT,   STDERR } opt;
        const char* options[] = {"cd", "env", "kill", "nice", "user", "stdout", "stderr"};
        
        if (eis.decodeTupleSize() != 2 || (int)(opt = (OptionT)eis.decodeAtomIndex(options, op)) < 0) {
            m_err << "badarg: cmd option must be an atom"; return -1;
        }
        
        switch (opt) {
            case CD:
            case KILL:
            case USER:
                // {cd, Dir::string()} | {kill, Cmd::string()}
                if (eis.decodeString(val) < 0) {
                    m_err << op << " bad option value"; return -1;
                }
                if      (opt == CD)     m_cd        = val;
                else if (opt == KILL)   m_kill_cmd  = val;
                else if (opt == USER) {
                    struct passwd *pw = getpwnam(val.c_str());
                    if (pw == NULL) {
                        m_err << "Invalid user " << val << ": " << ::strerror(errno);
                        return -1;
                    }
                    m_user = pw->pw_uid;
                }
                break;

            case NICE:
                if (eis.decodeInt(m_nice) < 0 || m_nice < -20 || m_nice > 20) {
                    m_err << "nice option must be an integer between -20 and 20"; 
                    return -1;
                }
                break;

            case ENV: {
                // {env, [NameEqualsValue::string()]}
                int env_sz = eis.decodeListSize();
                if (env_sz < 0) {
                    m_err << "env list expected"; return -1;
                } else if ((m_cenv = (const char**) new char* [env_sz+1]) == NULL) {
                    m_err << "out of memory"; return -1;
                }

                for (int i=0; i < env_sz; i++) {
                    std::string s;
                    if (eis.decodeString(s) < 0) {
                        m_err << "invalid env argument #" << i; return -1;
                    }
                    m_env.push_back(s);
                    m_cenv[i] = m_env.back().c_str();
                }
                m_cenv[env_sz] = NULL;
                break;
            }

            case STDOUT:
            case STDERR: {
                int type = 0, sz;
                std::string s, fop;
                type = eis.decodeType(sz);
                    
                if (type == ERL_ATOM_EXT)
                    eis.decodeAtom(s);
                else if (type == ERL_STRING_EXT)
                    eis.decodeString(s);
                else if (type == ERL_SMALL_TUPLE_EXT && sz == 2 && 
                         eis.decodeAtom(fop) == 0 && eis.decodeString(s) == 0 && fop == "append") {
                    ;
                } else {
                    m_err << "Atom, string or {'append', Name} tuple required for option " << op;
                    return -1;
                }

                std::string& rs = (opt == STDOUT) ? m_stdout : m_stderr;
                std::stringstream ss;
                int fd = (opt == STDOUT) ? 1 : 2;
                
                if (s == "null") {
                    ss << fd << ">/dev/null";
                    rs = ss.str();
                } else if (s == "stderr" && opt == STDOUT)
                    rs = "1>&2";
                else if (s == "stdout" && opt == STDERR)
                    rs = "2>&1";
                else if (s != "") {
                    ss << fd << (fop == "append" ? ">" : "") << ">\"" << s << "\"";
                    rs = ss.str();
                }
                break;
            }
            default:
                m_err << "bad option: " << op; return -1;
        }
    }

    if (m_stdout == "1>&2" && m_stderr != "2>&1") {
        m_err << "cirtular reference of stdout and stderr";
        return -1;
    } else if (!m_stdout.empty() || !m_stderr.empty()) {
        std::stringstream ss;
        ss << m_cmd;
        if (!m_stdout.empty()) ss << " " << m_stdout;
        if (!m_stderr.empty()) ss << " " << m_stderr;
        m_cmd = ss.str();
    }
    return 0;
}

/*
int CmdOptions::init(const std::list<std::string>& list) 
{
    int i, size=0;
    for(std::list<std::string>::iterator it=list.begin(), end=list.end(); it != end; ++it)
        size += it->size() + 1;
    if (m_env.resize(m_size) == NULL)
        return -1;
    m_count = list.size() + 1;
    char *p = m_env.c_str();
    for(std::list<std::string>::iterator it=list.begin(), end=list.end(); it != end; ++it) {
        strcpy(p, it->c_str());
        m_cenv[i++] = p;
        p += it->size() + 1;
    }
    m_cenv[i] = NULL;
    return 0;
}
*/
