#ifndef BUF_SIZE
#define BUF_SIZE 2048
#endif

#ifndef HONEYCOMB_H
#define HONEYCOMB_H

#ifdef __cplusplus

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
#include <errno.h>
#include <map>
#include <set>
#include <utility>
#include <unistd.h>

// System includes
#include <sys/stat.h>
#include <sys/resource.h>

using std::string;

// Erlang interface
#include "hc_support.h"

/*---------------------------- Defines ------------------------------------*/

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
  // ei::StringBuffer<256>   m_tmp;       // Temporary storage
  // ei::Serializer          m_eis;       // Erlang serializer
  std::string             m_root_dir;     // The root directory to start from
  std::string             m_run_dir;      // The directory to run bees and honeycombs from
  std::string             m_storage_dir;  // The directory to store the sleeping bees
  std::string             m_working_dir;  // Working directory
  
  mode_t                  m_mode;         // The mode of the m_working_dir
  std::string             m_name;         // Name
  std::string             m_sha;          // Sha
  std::string             m_app_type;     // The type of the application
    std::string             m_skel_dir;   // A skeleton choot directory to work from
  std::string             m_stdout;       // The stdout to use for the execution of the command
  std::string             m_stderr;       // The stderr to use for the execution of the command
  mount_type*             m_mount;        // A mount associated with the honeycomb
  std::string             m_image;        // The image to mount
  string_set              m_executables;  // Executables to be bundled in the honeycomb
  string_set              m_dirs;         // Directories to be included in the app
  string_set              m_files;        // Extra directories to be included in the honeycomb
  std::list<std::string>  m_env;          // A list of environment variables to use when starting
  int                     m_port;         // The port to run
    // Resource sets
    rlim_t                m_nofiles;      // Number of files
  long                    m_nice;         // The "niceness" level
  size_t                  m_size;         // The directory size
  uid_t                   m_user;         // run as user (generated if not given)
  gid_t                   m_group;        // run as this group
  char*                   m_cenv;         // The string list of environment variables
  // Internal
  std::string             m_scm_url;      // The url for the scm path (to clone from)
  unsigned int            m_cenv_c;       // The current count of the environment variables
  string_set              m_already_copied;
  honeycomb_config*       m_honeycomb_config; // We'll compute this on the app type

public:
  Honeycomb(std::string app_type, honeycomb_config *c) {
    new (this) Honeycomb(app_type);
    m_honeycomb_config = c;
    init();
  }
  Honeycomb(std::string app_type) : m_mount(NULL),m_nice(INT_MAX),m_size(0),m_user(-1),m_cenv(NULL),m_scm_url("") {
    m_root_dir = "/var/beehive";
    m_run_dir = m_root_dir + "/run";
    m_storage_dir = m_root_dir + "/storage";
    m_working_dir = m_root_dir + "/working";
    
    m_app_type = app_type;
  }
  Honeycomb() {
    new (this) Honeycomb("rack");
  }
  ~Honeycomb() { 
    delete [] m_cenv; 
    m_cenv = NULL;
  }
  
  const char*  working_dir()  const { return m_working_dir.c_str(); }
  const char*  scm_url()  const { return m_scm_url.c_str(); }
  const char*  run_dir()  const { return m_run_dir.c_str(); }
  const char*  skel()     const { return m_skel_dir.c_str(); }
  const char*  sha()      const { return m_sha.c_str(); }
  const char*  image()    const { return m_image.c_str(); }
  const char*  storage_dir() const { return m_storage_dir.c_str(); }
  const char*  name()    const { return m_name.c_str(); }
  int          port()     const { return m_port; }
  char* const* env()      const { return (char* const*)m_cenv; }
  string_set   executables() const { return m_executables; }
  string_set   directories() const { return m_dirs; }
  uid_t        user()     const { return m_user; }
  gid_t        group()    const { return m_group; }
  int          nice()     const { return m_nice; }
  mount_type*  type_of_mount() const { return m_mount; }
  const char*  app_type() const { return m_app_type.c_str(); }
  const honeycomb_config *config() const {return m_honeycomb_config; }
  
  // Setters
  void set_name(std::string n) { m_name = n; }
  void set_port(int p) { m_port = p; }
  void set_user(uid_t u) { m_user = u; }
  void set_image(std::string i) { m_image = i; }
  void set_config(honeycomb_config *c) {m_honeycomb_config = c;}
  void set_scm_url(std::string url) {m_scm_url = url;}
  void set_root_dir(std::string dir) {
    m_root_dir = dir;
    m_run_dir = m_root_dir + "/run";
    m_storage_dir = m_root_dir + "/storage";
    m_working_dir = m_root_dir + "/working";
  }
  void set_run_dir(std::string d) {m_run_dir = d;}
  void set_storage_dir(std::string d) {m_storage_dir = d;}
  
  void set_working_dir(std::string dir) {m_working_dir = dir;}
  void set_sha(std::string sha) { m_sha = sha; }
  void add_file(std::string file) { m_files.insert(file); }
  void add_dir(std::string dir) { m_dirs.insert(dir); }
  void add_executable(std::string exec) { m_executables.insert(exec); }
  
  // Actions
  int bundle(int debug_level);
  int start(int debug_level);
  int stop(int debug_level);
  int mount(int debug_level);
  int unmount(int debug_level);
  int cleanup(int debug_level);
  
  int valid();
    
private:
  void init();
  void setup_internals();
  std::string find_binary(const std::string& file);
  bool abs_path(const std::string & path);
  uid_t random_uid();
  int setup_defaults();
  int build_env_vars();
  const char * const to_string(long long int n, unsigned char base);
  int temp_drop();
  int perm_drop();
  int restore_perms();
  // Limits
  void set_rlimits();
  int set_rlimit(const int res, const rlim_t limit);
  // Building
  int comb_exec(std::string cmd, bool should_wait); // Run a hook on the system
  void exec_hook(std::string action, int stage, phase *p);
  int run_in_fork_and_maybe_wait(char *argv[], char* const* env, bool should_wait);
  void ensure_exists(std::string s);
  string_set *string_set_from_lines_in_file(std::string filepath);
};

/**
 * Bee
 **/
class Bee {
public:
  pid_t           cmd_pid;        // Pid of the custom kill command
  // ei::TimeVal     deadline;       // Time when the <cmd_pid> is supposed to be killed using SIGTERM.
  bool            sigterm;        // <true> if sigterm was issued.
  bool            sigkill;        // <true> if sigkill was issued.

  Bee() : cmd_pid(-1), sigterm(false), sigkill(false) {}
  ~Bee() {}
  
  Bee(const Honeycomb& hc, pid_t _cmd_pid) {
    cmd_pid = _cmd_pid;
  }
  
};

/*--------------------------------------------------------------------------*/

#endif
#endif