// Core includes
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <signal.h>

// Object includes
#include <pwd.h>
#include <map>
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
#include <setjmp.h>
#include <limits.h>

// Our own includes
#include "chroot.h"
using namespace std;

//-------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------

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

    CmdOptions() : m_tmp(0, 256), m_image(NULL), m_nice(INT_MAX), m_size(0), m_user(INT_MAX), m_cenv(NULL) {}
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

const char * const to_string(long long int n, unsigned char base) {
  static char bfr[32];

  const char * frmt = "%lld";
  if (16 == base)     frmt = "%llx";
  else if (8 == base) frmt = "%llo";

  snprintf(bfr, sizeof(bfr) / sizeof(bfr[0]), frmt, n);
  return bfr;
}

int build_and_execute_chroot(CmdOptions& op, ei::StringBuffer<128> err) {
  
  if (!(op.user())) {
    err.write("Cannot set effective user to %d", op.user());
    perror(err.c_str());
    return EXIT_FAILURE;
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