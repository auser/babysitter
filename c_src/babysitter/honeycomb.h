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
#include "comb_process.h"
#include "hc_support.h"
#include "babysitter_types.h"

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
typedef void(*func_ptr)();

typedef enum {UNKNOWN, FAILURE} failure_type_t;

class Honeycomb;

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

/*--------------------------------------------------------------------------*/
 
/*---------------------------- Class definitions ---------------------------*/
/**
 * Honeycomb
 **/
class Honeycomb {
private:
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
  int                     m_port;         // The port to run
    // Resource sets
    rlim_t                m_nofiles;      // Number of files
  long                    m_nice;         // The "niceness" level
  size_t                  m_size;         // The directory size
  uid_t                   m_user;         // run as user (generated if not given)
  gid_t                   m_group;        // run as this group
  const char**            m_cenv;         // The string list of environment variables
  // Internal
  std::string             m_script_file;  // The script used to launch this honeycomb
  std::string             m_scm_url;      // The url for the scm path (to clone from)
  unsigned int            m_cenv_c;       // The current count of the environment variables
  string_set              m_already_copied;
  honeycomb_config*       m_honeycomb_config; // We'll compute this on the app type
  
  int                     m_debug_level;  // Only for internal usage

public:
  Honeycomb(std::string app_type, honeycomb_config *c) {
    new (this) Honeycomb(app_type);
    set_config(c);
  }
  Honeycomb(std::string app_type) : m_mount(NULL),m_port(80),m_nice(INT_MAX),m_size(0),m_user(-1),m_cenv(NULL),m_scm_url(""),m_debug_level(0) {
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
    m_cenv = NULL;
  }
  
  const char*   working_dir()  const { return m_working_dir.c_str(); }
  const char*   scm_url()  const { return m_scm_url.c_str(); }
  const char*   run_dir()  const { return m_run_dir.c_str(); }
  const char*   skel()     const { return m_skel_dir.c_str(); }
  const char*   sha()      const { return m_sha.c_str(); }
  const char*   image()    const { return m_image.c_str(); }
  const char*   storage_dir() const { return m_storage_dir.c_str(); }
  const char*   name()    const { return m_name.c_str(); }
  int           port()     const { return m_port; }
  char* const*  env()      const { return (char* const*)m_cenv; }
  string_set    executables() const { return m_executables; }
  string_set    directories() const { return m_dirs; }
  uid_t         user()     const { return m_user; }
  gid_t         group()    const { return m_group; }
  int           nice()     const { return m_nice; }
  mount_type*   type_of_mount() const { return m_mount; }
  const char*   app_type() const { return m_app_type.c_str(); }
  const         honeycomb_config *config() const {return m_honeycomb_config; }  
    
  // Setters
  void set_name(std::string n) { m_name = n; }
  void set_app_type(std::string t) {m_app_type = t;}
  void set_debug_level(int d) { m_debug_level = d; }
  void set_port(int p) { m_port = p; }
  void set_user(uid_t u) { m_user = u; }
  void set_group(gid_t g) { m_group = g; }
  void set_config(honeycomb_config *c) {
    m_honeycomb_config = c;
    init();
  }
  void set_scm_url(std::string url) {m_scm_url = url;}
  void set_root_dir(std::string dir) {
    m_root_dir = replace_vars_with_value(dir);
    m_run_dir     = dir + "/run";
    m_storage_dir = dir + "/storage";
    m_working_dir = dir + "/working";
  }
  void set_run_dir(std::string d) {m_run_dir = d;}
  void set_storage_dir(std::string d) { m_storage_dir = d; }
  void set_working_dir(std::string dir) { m_working_dir = dir; }
  void set_image(std::string i) { m_image = i; }
  void set_skel_dir(std::string i) { m_skel_dir = i; }
  
  void set_sha(std::string sha) { m_sha = sha; }
  void add_file(std::string file) { m_files.insert(file); }
  void add_dir(std::string dir) { m_dirs.insert(dir); }
  void add_executable(std::string exec) { m_executables.insert(exec); }
  
  // Actions
  int bundle();
  int start(MapChildrenT &children); 
  int stop(Bee bee, MapChildrenT &children);
  int mount();
  int unmount();
  int cleanup();
  
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
  int comb_exec(std::string cmd, std::string cd, CombProcess* process);
  void exec_hook(std::string action, int stage, phase *p, std::string cd);
  pid_t run_in_fork_and_wait(char *argv[], char* const* env, std::string cd, int running_script);
  void ensure_exists(std::string s);
  std::string replace_vars_with_value(std::string original);
  std::string map_char_to_value(std::string f_name);
  string_set *string_set_from_lines_in_file(std::string filepath);
};

/**
 * Bee
 **/
class Bee {
public:
  pid_t           m_pid;        // Pid of the custom kill command
  Honeycomb       m_hc;             // Honeycomb
  // ei::TimeVal     deadline;       // Time when the <m_pid> is supposed to be killed using SIGTERM.
  bool            sigterm;        // <true> if sigterm was issued.
  bool            sigkill;        // <true> if sigkill was issued.
  bee_status      m_status;         // Status of the bee

  Bee() : m_pid(-1), sigterm(false), sigkill(false), m_status(BEE_RUNNING) {}
  ~Bee() {}
  
  Bee(const Honeycomb& _hc, pid_t _m_pid) {
    new(this) Bee();
    m_pid = _m_pid;
    m_hc    = _hc;
  }
  
  // Accessors
  const char*   name()    const { return m_hc.name(); }
  Honeycomb     honeycomb() const { return m_hc; }
  const char*   status()  const {
    switch(m_status) {
      case BEE_RUNNING:
      return "running";
      break;
      default:
      return "unknown";
      break;
    }
  }
  
  // Setters
  void set_status(bee_status s) {m_status = s;}
  
public:
  int stop();
};

/*--------------------------------------------------------------------------*/

#endif
#endif