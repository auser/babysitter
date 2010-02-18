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
#include <string.h>
#include <map>
#include <set>
#include <utility>

// System includes
#include <sys/stat.h>
#include <sys/resource.h>

#include <gelf.h>

using std::string;

// Erlang interface
#include <ei.h>
#include "ei++.h"

#include "hc_support.h"

/*---------------------------- Defines ------------------------------------*/
#ifndef BUF_SIZE
#define BUF_SIZE 2048
#endif


// #define DEBUG 0

#ifdef DEBUG
#define DEBUG_MSG printf
#else
#define DEBUG_MSG(args...)
#endif

#ifndef FS_SLASH
#define FS_SLASH '/'
#endif

#ifndef BEFORE
#define BEFORE 1001
#endif
#ifndef AFTER
#define AFTER 1002
#endif

/*---------------------------- TYPES ---------------------------------------*/
typedef std::set<std::string> string_set;

typedef enum {UNKNOWN, FAILURE} failure_type_t;

class Honeycomb;
class Bee;

// TODO: Implement multiple mounts for bees
typedef enum _mount_types_ {
  MOUNT_IMAGE,
  MOUNT_PACKAGED
} mount_types;

typedef struct _mount_ {
  mount_types mount_type;
  string src;
  string dest;
} mount_type;

typedef struct _limits_ {
  rlim_t AS;
  rlim_t CORE;
  rlim_t DATA;
  rlim_t NOFILE;
  rlim_t FSIZE;
  rlim_t MEMLOCK;
  rlim_t RSS;
  rlim_t STACK;
  rlim_t CPU;
} resource_limits;

typedef unsigned char byte;
typedef int   exit_status_t;
typedef pid_t kill_cmd_pid_t;
typedef std::pair<pid_t, exit_status_t>     PidStatusT;
typedef std::pair<pid_t, Bee>         PidInfoT;
typedef std::map <pid_t, Bee>         MapChildrenT;
typedef std::pair<kill_cmd_pid_t, pid_t>    KillPidStatusT;
typedef std::map <kill_cmd_pid_t, pid_t>    MapKillPidT;
typedef std::map <std::string, honeycomb_config*> ConfigMapT;

/*--------------------------------------------------------------------------*/
 
/*---------------------------- Class definitions ---------------------------*/

/**
* Honeycomb config
* 
* Format: 
* # This is a comment
* bundle.before : executable_script
* bundle : executable_script
* bundle.after : executable_script
* files : 
* executables : /usr/bin/ruby irb cat
* directories : /var/lib/gems/1.8
*
**/
// class HoneycombConfig {
// public:
//   size_t num_headers;
//   std::string config_file;
//   string_set  executables;
//   string_set  dirs;
//   string_set  files;
//   
//   HoneycombConfig(std::string file) : config_file(file) {
//     init();
//   }
//   HoneycombConfig() : config_file("") {}
//   ~HoneycombConfig() {}
// 
// private:
//   void init();
//   void parse();
//   int parse_honeycomb_config_file();
// };

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
  std::string             m_root_dir;  // The root directory to start from
  std::string             m_run_dir;   // The directory to run bees and honeycombs from
  mode_t                  m_mode;      // The mode of the m_cd
  std::string             m_cd;        // The directory to execute the command (generated, if not given)
  std::string             m_app_type;  // The type of the application
    std::string             m_skel;      // A skeleton choot directory to work from
  std::string             m_stdout;    // The stdout to use for the execution of the command
  std::string             m_stderr;    // The stderr to use for the execution of the command
  mount_type*             m_mount;     // A mount associated with the honeycomb
  std::string             m_image;     // The image to mount
  string_set              m_executables; // Executables to be bundled in the honeycomb
  string_set              m_dirs;      // Directories to be included in the app
  string_set              m_files;     // Extra directories to be included in the honeycomb
  std::list<std::string>  m_env;       // A list of environment variables to use when starting
    // Resource sets
    rlim_t                m_nofiles;     // Number of files
  long                    m_nice;      // The "niceness" level
  size_t                  m_size;      // The heap/stack size
  uid_t                   m_user;      // run as user (generated if not given)
  gid_t                   m_group;     // run as this group
  const char**            m_cenv;      // The string list of environment variables
  // Internal
  std::string             m_scm_url;   // The url for the scm path (to clone from)
  int                     m_cenv_c;    // The current count of the environment variables
  string_set              m_already_copied;
  honeycomb_config*       m_honeycomb_config; // We'll compute this on the app type

public:
  Honeycomb(std::string app_type, honeycomb_config *c) {
    new (this) Honeycomb(app_type);
    m_honeycomb_config = c;
    init();
  }
  Honeycomb(std::string app_type) : m_tmp(0,256),m_cd(""),m_mount(NULL),m_nice(INT_MAX),m_size(0),m_cenv(NULL),m_scm_url("") {
    m_app_type = app_type;
  }
  Honeycomb() {
    new (this) Honeycomb("rack");
  }
  ~Honeycomb() { 
    delete [] m_cenv; 
    m_cenv = NULL;
  }
  
  const char*  strerror() const { return m_err.str().c_str(); }
  const char*  cmd()      const { return m_cmd.c_str(); }
  const char*  cd()       const { return m_cd.c_str(); }
  const char*  scm_url()  const { return m_scm_url.c_str(); }
  const char*  run_dir()  const { return m_run_dir.c_str(); }
  const char*  skel()     const { return m_skel.c_str(); }
  char* const* env()      const { return (char* const*)m_cenv; }
  const char*  kill_cmd() const { return m_kill_cmd.c_str(); }
  string_set   executables() const { return m_executables; }
  string_set   directories() const { return m_dirs; }
  uid_t        user()     const { return m_user; }
  gid_t        group()    const { return m_group; }
  int          nice()     const { return m_nice; }
  mount_type*  mount()    const { return m_mount; }
  const char*  app_type() const { return m_app_type.c_str(); }
  const honeycomb_config *config() const {return m_honeycomb_config; }
  
  int ei_decode(ei::Serializer& ei);
  int valid();
  // Actions
  int bundle();
  
  void set_config(honeycomb_config *c) {m_honeycomb_config = c;}
  void set_scm_url(std::string url) {m_scm_url = url;}
  
private:
  void init();
  pid_t execute();
  uid_t random_uid();
  int setup_defaults();
  const char * const to_string(long long int n, unsigned char base);
  int temp_drop();
  int perm_drop();
  int restore_perms();
  std::pair<string_set*, string_set*> * dynamic_loads(Elf *elf);
  // Limits
  void set_rlimits();
  int set_rlimit(const int res, const rlim_t limit);
  // Building
  int comb_exec(std::string cmd); // Run a hook on the system
  void exec_hook(std::string action, int stage, phase *p);
  void ensure_cd_exists();
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