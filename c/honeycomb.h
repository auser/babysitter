/**
 * Honeycomb
 * This class keeps all the fun little variables that are 
 * associated with a launch environment. This includes all the variables
 * that are known such as image files to mount, chroot users, 
 * start_commands, pids, etc.
 *
 * Bee
 * This class keeps all the runtime information about a process running inside a 
 * honeycomb, including the status, pid information, etc.
 **/
/*---------------------------- Includes ------------------------------------*/
// STL Includes
#include <list>
#include <sstream>
#include <string>
#include <map>
#include <set>
#include <utility>

// Erlang interface
#include <ei.h>
#include "ei++.h"

/*---------------------------- Defines ------------------------------------*/
#define BUF_SIZE 2048

/*---------------------------- TYPES ---------------------------------------*/
typedef std::set<std::string> string_set;

typedef enum {UNKNOWN, FAILURE} failure_type_t;

class Honeycomb;
class Bee;

// TODO: Implement multiple mounts for bees
//  and 
typedef enum _mount_types_ {
  MOUNT_IMAGE,
  MOUNT_PACKAGED
} mount_types;

typedef struct _mount_ {
  mount_types mount_type;
  std::string src;
  std::string dest;
} mount_type;

typedef unsigned char byte;
typedef int   exit_status_t;
typedef pid_t kill_cmd_pid_t;
typedef std::pair<pid_t, exit_status_t>     PidStatusT;
typedef std::pair<pid_t, Bee>         PidInfoT;
typedef std::map <pid_t, Bee>         MapChildrenT;
typedef std::pair<kill_cmd_pid_t, pid_t>    KillPidStatusT;
typedef std::map <kill_cmd_pid_t, pid_t>    MapKillPidT;

/*--------------------------------------------------------------------------*/
 
/*---------------------------- Class definitions ---------------------------*/

/**
 * Honeycomb
 **/
class Honeycomb {
private:
  ei::StringBuffer<256>   m_tmp;       // Temporary storage
  ei::Serializer          m_eis;       // Erlang serializer
  std::stringstream       m_err;       // Error message to use to pass backwards to the erlang caller
  std::string             m_cmd;       // The command to execute to start
  std::string             m_kill_cmd;  // A special command to kill the process (if needed)
  std::string             m_cd;        // The directory to execute the command (generated, if not given)
  std::string             m_stdout;    // The stdout to use for the execution of the command
  std::string             m_stderr;    // The stderr to use for the execution of the command
  mount_type*             m_mount;     // A mount associated with the honeycomb
  std::list<std::string>  m_env;       // A list of environment variables to use when starting
  long                    m_nice;      // The "niceness" level
  size_t                  m_size;      // The heap/stack size
  int                     m_user;      // run as user (generated if not given)
  const char**            m_cenv;      // The string list of environment variables

public:
  Honeycomb() : m_tmp(0,256),m_mount(NULL),m_nice(INT_MAX),m_size(0),m_user(INT_MAX),m_cenv(NULL) {
    ei::Serializer m_eis(2);
  }
  ~Honeycomb() { delete [] m_cenv; m_cenv = NULL; }
  
  const char*  strerror() const { return m_err.str().c_str(); }
  const char*  cmd()      const { return m_cmd.c_str(); }
  const char*  cd()       const { return m_cd.c_str(); }
  char* const* env()      const { return (char* const*)m_cenv; }
  const char*  kill_cmd() const { return m_kill_cmd.c_str(); }
  int          user()     const { return m_user; }
  int          nice()     const { return m_nice; }
  mount_type*  mount()    const { return m_mount; }
  
  int ei_decode(ei::Serializer& ei);
};

/**
 * Bee
 **/
class Bee {
public:
  std::string     cmd;            // Executed command
  mount_type*     mount_point;    // Mount-point related to this process
  pid_t           cmd_pid;        // Pid of the custom kill command
  std::string     kill_cmd;       // Kill command to use (if provided - otherwise use SIGTERM)
  kill_cmd_pid_t  kill_cmd_pid;   // Pid of the command that <pid> is supposed to kill
  ei::TimeVal     deadline;       // Time when the <cmd_pid> is supposed to be killed using SIGTERM.
  bool            sigterm;        // <true> if sigterm was issued.
  bool            sigkill;        // <true> if sigkill was issued.

  Bee() : cmd_pid(-1), kill_cmd_pid(-1), sigterm(false), sigkill(false) {}
  ~Bee() {}
  
  Bee(const Honeycomb& hc, pid_t _cmd_pid) {
    cmd = hc.cmd();
    mount_point = hc.mount();
    cmd_pid = _cmd_pid;
    kill_cmd = hc.kill_cmd();
  }
  
};

/*--------------------------------------------------------------------------*/