/*---------------------------- Includes ------------------------------------*/
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <assert.h>
#include <fcntl.h>
#include <regex.h>
#include <libgen.h>
// System
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/wait.h>

// Include sys/capability if the system can handle it. 
#ifdef HAVE_SYS_CAPABILITY_H
#include <sys/capability.h>
#endif
// Erlang interface
// #include "ei++.h"

#include "honeycomb_config.h"
#include "honeycomb.h"
#include "hc_support.h"
#include "babysitter_utils.h"

/*---------------------------- Implementation ------------------------------*/

// using namespace ei;
#define DEFAULT_PATH "/bin:/usr/bin:/usr/local/bin:/sbin;"

std::string Honeycomb::map_char_to_value(std::string f_name) {
  if (f_name == "APP_NAME") return m_name;
  else if (f_name == "BEE_SHA") return m_sha;
  else if (f_name == "ROOT_DIR") return m_root_dir;
  else if (f_name == "STORAGE_DIR") return m_storage_dir;
  else if (f_name == "RUN_DIR") return m_run_dir;
  else
    return NULL;
}

int Honeycomb::setup_defaults() {
  return 0;
}

/**
 * This will replace BASH-like variables with the corresponding function on the Honeycomb
**/
std::string Honeycomb::replace_vars_with_value(std::string original) {
  std::string var_str, working_string, replace_value;
  char working_char;
  unsigned int curr_pos, original_location;
  unsigned int len = original.length();
  
  curr_pos = 0;
  
  while (curr_pos < len) {
    var_str = ""; original_location = curr_pos;
    if (original[curr_pos] == '$') {
      original_location = curr_pos++;
      while( (original[curr_pos] != '\0') && (((original[curr_pos] > 64) && (original[curr_pos] < 91)) || (original[curr_pos] == 95)) ) {
        // The only "allowed" character that's not a string is '_'
        
        var_str.push_back(original[curr_pos]);
        
        working_char = original[curr_pos++];
      }
      
      replace_value = map_char_to_value(var_str);
      
      working_string += replace_value;
    } else
      working_string.push_back(original[curr_pos++]);
  }
  
  return working_string;
}


int Honeycomb::build_env_vars() {
  /* Setup environment defaults */
  std::string pth = DEFAULT_PATH;
  
  std::string extension ("ext3");
  
  if (m_image != "") {
    size_t period = m_image.rfind('.');
    if (period != std::string::npos) extension = m_image.substr(period+1, m_image.length());
  }
  
  std::string usr_p   (to_string(m_user, 10));
  std::string group_p (to_string(m_group, 10));
  std::string port_p (to_string(m_port, 10));
  char size_buf[BUF_SIZE]; memset(size_buf, 0, BUF_SIZE); sprintf(size_buf, "%d", (int)(m_size / 1024));
  std::string size_p  (size_buf);
  // free(size_buf);
  
  string_set *envs = new string_set();
  envs->insert("APP_TYPE=" + m_app_type);
  envs->insert("APP_USER=" + usr_p);
  envs->insert("APP_GROUP=" + group_p);
  envs->insert("BEE_SHA=" + m_sha);
  envs->insert("BEE_SIZE=" + size_p);

  envs->insert("APP_NAME=" + m_name);
  envs->insert("SCM_URL=" + m_scm_url);
  envs->insert("BEE_PORT=" + port_p);
  envs->insert("BEE_IMAGE=" + m_image);
  envs->insert("WORKING_DIR=" + m_working_dir);

  envs->insert("STORAGE_DIR=" + m_storage_dir);
  envs->insert("RUN_DIR=" + m_run_dir);
  envs->insert("PATH=" + pth);
  envs->insert("LD_LIBRARY_PATH=/lib;/usr/lib;/usr/local/lib");
  envs->insert("HOME=home/app");  
  envs->insert("FILESYSTEM=" + extension);
  
  char* default_env_vars[] = { NULL };
  
  unsigned int i = 0;
  int total_len = 0;
  int templen;
  size_t found; 
  std::string working_str;
  
  for (string_set::iterator var = envs->begin(); var != envs->end(); var++) {
    // Not crazy with creating a new string everytime, but... it'll work for now
    working_str = var->c_str();
    
    found = working_str.find('$');
    if (found != std::string::npos) {
      working_str = replace_vars_with_value(working_str);
    }
    
    templen = strlen(working_str.c_str());
    if ((default_env_vars[i] = (char *) malloc(sizeof(char *) * templen)) == NULL ) {
      fprintf(stderr, "Could not malloc memory for env vars: %s\n", ::strerror(errno));
      exit(-1);
    }
    memset(default_env_vars[i], 0, templen);
    strncpy(default_env_vars[i], working_str.c_str(), (int)templen);
    //default_env_vars[i][templen] = '\0'; // NULL terminate the string
    total_len += templen; // Save ourselves a few cycles of computation later
    i++;
  }
  
  default_env_vars[i] = NULL;
  m_cenv_c = i;
  
  total_len = total_len * sizeof(char);
  
  if ( (m_cenv = (const char**) malloc( total_len )) == NULL ) {
    fprintf(stderr, "Could not allocate a new char. Out of memory\n");
    exit(-1);
  }
    
  memset(m_cenv, 0, total_len );
  memcpy(m_cenv, default_env_vars, total_len);
  
  if (m_debug_level > 2) {
    printf("Environment variables:\n");
    for (i = 0; i < m_cenv_c; i++) { printf("\t%s\n", m_cenv[i]); }
  }
  
  return 0;
}

#ifndef MAX_ARGS
#define MAX_ARGS 64
#endif
// Run a hook on the system
int Honeycomb::comb_exec(std::string cmd, bool should_wait = true) {
  setup_defaults(); // Setup default environments
  build_env_vars();
  
  const std::string shell = getenv("SHELL");  
  const std::string shell_args = "-c";
  const char* argv[] = { shell.c_str(), shell_args.c_str(), cmd.c_str(), NULL };
  bool running_script = false;
  char filename[40];
  
  // If we are looking at shell script text
  if ( strncmp(cmd.c_str(), "#!", 2) == 0 ) {
    int size, fd;
    // Note for the future cleanup, that we'll be running a script to cleanup
    running_script = true;
    
    snprintf(filename, 40, "/tmp/babysitter.XXXXXXXXX");
    
    // Make a tempfile in the filename format
    if ((fd = mkstemp(filename)) == -1) {
      fprintf(stderr, "Could not open tempfile: %s\n", filename);
      return -1;
    }
    
    size = strlen(cmd.c_str());
    // Print the command into the file
    if (write(fd, cmd.c_str(), size) == -1) {
      fprintf(stderr, "Could not write command to tempfile: %s\n", filename);
      return -1;
    }
    
    // Confirm that the command is written
    if (fsync(fd) == -1) {
      fprintf(stderr, "fsync failed for tempfile: %s\n", filename);
      return -1;
    }
    
    close(fd);
    
    // Modify the command to match call the filename
    std::string sFile (filename);
    
		if (chown(sFile.c_str(), m_user, m_group) != 0) {
#ifdef DEBUG
     fprintf(stderr, "Could not change owner of '%s' to %d\n", sFile.c_str(), m_user);
#endif
		}

    // Make it executable
    if (chmod(sFile.c_str(), 040700) != 0) {
      fprintf(stderr, "Could not change permissions to '%s' %o\n", sFile.c_str(), 040750);
    }
    
    // Run in a new process
    argv[0] = sFile.c_str();
    argv[1] = NULL;    
  } else {
    
    // First, we have to construct the command
    int argc = 0;
    char str_cmd[BUF_SIZE];
    memset(str_cmd, 0, BUF_SIZE); // Clear it
    
    memcpy(str_cmd, cmd.c_str(), strlen(cmd.c_str()));
    
    argv[argc] = strtok(str_cmd, " \r\t\n");
    // Remove the first one
    std::string bin = find_binary(argv[argc]);
    argv[argc] = bin.c_str();
    
    // argv[argc] = strtok(str_cmd, " \r\t\n");
    
    while (argc++ < MAX_ARGS) 
      if (! (argv[argc] = strtok(NULL, " \t\n"))) break;
    
    printf("argv[0] = %s\n", argv[0]);
  }
  
  printf("Running... %s\n", argv[0]);
  // Run in a new process
  run_in_fork_and_maybe_wait((char **)argv, (char * const*) m_cenv, should_wait);
  
  if (running_script) unlink(filename);
  return 0;
}

// Execute a hook
void Honeycomb::exec_hook(std::string action, int stage, phase *p) {
  if (stage == BEFORE) {
    if (p->before) comb_exec(p->before); //printf("Run before hook for %s: %s\n", action.c_str(), p->before);
  } else if (stage == AFTER) {
    if (p->after) comb_exec(p->after); //printf("Run after hook for %s %s\n", action.c_str(), p->after);
  } else {
    printf("Unknown hook for: %s %d\n", action.c_str(), stage);
  }
}

void Honeycomb::ensure_exists(std::string dir) {
  mkdir_p(dir, m_user, m_group, m_mode);
}

string_set *Honeycomb::string_set_from_lines_in_file(std::string filepath) {
  string_set *lines = new string_set();
  FILE *fp;
  char line[BUF_SIZE];
  int len;
  
  // If we weren't given a file
  if (filepath == "") {
    fprintf(stderr, "string_set_from_lines_in_file:");
    return lines;
  }
  
  // Open the file
  if ((fp = fopen(filepath.c_str(), "r")) == NULL) {
    fprintf(stderr, "Could not open '%s'\n", filepath.c_str());
    return lines;
  }
    
  memset(line, 0, BUF_SIZE);
  
  while ( fgets(line, BUF_SIZE, fp) != NULL) {
    len = (int)strlen(line) - 1;
    
    // Chomp the beginning of the line
    for(int i = 0; i < len; ++i) {
      if ( isspace(line[i]) ) {        
        for (int j = 0; j < len; ++j) {
          line[j] = line[j+1];
        }
        line[len--] = 0;
      } else {
        break;
      }
    }
    // Chomp the end of the line
    while ((len>=0) && (isspace(line[len])) ) {
      line[len] = 0;
      len--;
    }
    // Skip newlines
    if (line[0] == '\n') continue;
    // Insert 
    lines->insert(line);
  }
  
  return lines;
}

void Honeycomb::setup_internals()
{
  // The m_working_dir path is the confinement_root plus the user's uid
  // because it's randomly generated
  std::string usr_p (to_string(m_user, 10));
  std::string usr_postfix = m_name + "/" + usr_p;
  
  m_working_dir = m_working_dir + "/" + usr_postfix;
  // m_storage_dir = m_storage_dir + "/" + usr_postfix;
}

int Honeycomb::run_in_fork_and_maybe_wait(char *argv[], char* const* env, bool should_wait) {
  int status;
  pid_t pid;
  
  // Run in a new process
  if ((pid = fork()) == -1) {
    perror("fork");
    return -1;
  }
  if (pid == 0) {
    // we are in the child process
    if (execve(argv[0], (char* const*)argv, (char* const*)m_cenv) < 0) {
      fprintf(stderr, "Cannot execute '%s' because '%s'\n", argv[0], ::strerror(errno));
      return -1;
    }
  }

  while (wait(&status) != pid) ; // We don't want to continue until the child process has ended
  assert(0 == status);
  
  return 0;
}

//---
// ACTIONS
//---

int Honeycomb::bundle() {
  setup_internals();
  debug(m_debug_level, 3, "Finding the bundle phase in our config (%p)\n", m_honeycomb_config);
  phase *p = find_phase(m_honeycomb_config, T_BUNDLE);
  
  debug(m_debug_level, 3, "Found the phase for the bundling action: %p\n", p);
  
  debug(m_debug_level, 3, "Running before hook for bundling\n");
  
  exec_hook("bundle", BEFORE, p);
  // Run command
  //--- Make sure the directory exists
  m_working_dir = replace_vars_with_value(m_working_dir);
  debug(m_debug_level, 3, "Making sure the working directory: '%s' exists\n", m_working_dir.c_str());
  
  ensure_exists(m_working_dir);
  
  temp_drop();
  
  // Change into the working directory
  debug(m_debug_level, 4, "Dropping into directory: %s\n", m_working_dir.c_str());
  if (chdir(m_working_dir.c_str())) {
    perror("chdir:");
    return -1;
  }
  
  // Clone the app in the directory if there is an scm_url
  // Get the clone command
  if (m_scm_url != "") {
    debug(m_debug_level, 2, "Cloning from %s\n", m_scm_url.c_str());
    std::string clone_command ("git clone --depth 0 %s home/app");
    
    if (m_honeycomb_config->clone_command != NULL) clone_command = m_honeycomb_config->clone_command;
    char str_clone_command[256];
    // The str_clone_command looks like:
    //  /usr/bin/git clone --depth 0 [git_url] [m_working_dir]/home/app
    snprintf(str_clone_command, 256, clone_command.c_str(), m_scm_url.c_str());
    // Do the cloning
    debug(m_debug_level, 3, "Cloning with command %s\n", clone_command.c_str());
    // Do not like me some system(3)
    int argc = 0;
    const char* argv[] = { NULL };
    
    // Get the binary
    std::string bin = find_binary("git");
    argv[argc] = bin.c_str();
    // Remove the first one
    strtok(str_clone_command, " \r\t\n");

    while (argc++ < MAX_ARGS) if (! (argv[argc] = strtok(NULL, " \t\n")) ) break;
    
    char buf[60];
    memset(buf, 0, 60);
    getwd(buf);
    printf("We are in '%s' dir\n", buf);
    
    run_in_fork_and_maybe_wait((char **)argv, (char * const*) m_cenv, true);
    
    std::string git_root_dir = m_working_dir + "/home/app";
    m_sha = parse_sha_from_git_directory(git_root_dir);
    m_size = dir_size_r(m_working_dir.c_str());
  }
  
  if ((p != NULL) && (p->command != NULL)) {
    debug(m_debug_level, 1, "Running client code: %s\n", p->command);
    // Run the user's command
    comb_exec(p->command);
  } else {
    //Default action
    printf("Running default action for bundle\n");
  }
  restore_perms();
  
  exec_hook("bundle", AFTER, p);
  
  debug(m_debug_level, 2, "Removing working directory: %s\n", working_dir());
  rmdir_p(m_working_dir);
  
  return 0;
}

int Honeycomb::start() 
{
  setup_internals();
  debug(m_debug_level, 3, "Finding the bundle phase in our config (%p)\n", m_honeycomb_config);
  phase *p = find_phase(m_honeycomb_config, T_START);
  
  debug(m_debug_level, 3, "Found the phase for the bundling action: %p\n", p);
  
  debug(m_debug_level, 3, "Running before hook for bundling\n");
  
  exec_hook("start", BEFORE, p);
  // Run command
  //--- Make sure the directory exists
  ensure_exists(m_run_dir = replace_vars_with_value(m_run_dir));
  
  temp_drop();
  
  // Change into the working directory
  if (chdir(m_run_dir.c_str())) {
    perror("chdir:");
    return -1;
  }
  
  if ((p != NULL) && (p->command != NULL)) {
    debug(m_debug_level, 1, "Running client code: %s\n", p->command);
    // Run the user's command
    comb_exec(p->command, true);
  } else {
    //Default action
    printf("Running default action for bundle\n");
  }
  
  restore_perms();
  
  exec_hook("start", AFTER, p);
  return 0;
}
int Honeycomb::stop()
{
  return 0;
}
int Honeycomb::mount()
{
  return 0;
}
int Honeycomb::unmount()
{
  return 0;
}
int Honeycomb::cleanup()
{
  return 0;
}
void Honeycomb::set_rlimits() {
  if(m_nofiles) set_rlimit(RLIMIT_NOFILE, m_nofiles);
}

int Honeycomb::set_rlimit(const int res, const rlim_t limit) {
  struct rlimit lmt = { limit, limit };
  if (setrlimit(res, &lmt)) {
    fprintf(stderr, "Could not set resource limit: %d\n", res);
    return(-1);
  }
  return 0;
}

/*---------------------------- UTILS ------------------------------------*/

const char *DEV_RANDOM = "/dev/urandom";
uid_t Honeycomb::random_uid() {
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

  DEBUG_MSG("Could not generate a good random UID after 10 attempts. Bummer!");
  return(-1);
}

std::string Honeycomb::find_binary(const std::string& file) {
  // assert(geteuid() != 0 && getegid() != 0); // We can't be root to run this.
  
  if (file == "") return "";
  
  if (abs_path(file)) return file;
  
  std::string pth = DEFAULT_PATH;
  char *p2 = getenv("PATH");
  if (p2) {pth = p2;}
  
  std::string::size_type i = 0;
  std::string::size_type f = pth.find(":", i);
  do {
    std::string s = pth.substr(i, f - i) + "/" + file;
    
    if (0 == access(s.c_str(), X_OK)) {return s;}
    i = f + 1;
    f = pth.find(':', i);
  } while(std::string::npos != f);
  
  if (!abs_path(file)) {
    fprintf(stderr, "Could not find the executable %s in the $PATH\n", file.c_str());
    return NULL;
  }
  if (0 == access(file.c_str(), X_OK)) return file;
  
  fprintf(stderr, "Could not find the executable %s in the $PATH\n", file.c_str());
  return NULL;
}

bool Honeycomb::abs_path(const std::string & path) {
  return FS_SLASH == path[0] || ('.' == path[0] && FS_SLASH == path[1]);
}

// Turn to string, by base
const char * const Honeycomb::to_string(long long int n, unsigned char base) {
  static char bfr[32];
  const char * frmt = "%lld";
  if (16 == base) {frmt = "%llx";} else if (8 == base) {frmt = "%llo";}
  snprintf(bfr, sizeof(bfr) / sizeof(bfr[0]), frmt, n);
  return bfr;
}

/*---------------------------- Permissions ------------------------------------*/

int Honeycomb::temp_drop() {

#ifdef HAS_SETRESGID
  DEBUG_MSG("Dropping into '%d' user\n", (int)m_user);
  if (setresgid(-1, m_user, getegid()) || setresuid(-1, m_user, geteuid()) || getegid() != m_user || geteuid() != m_user ) {
    fprintf(stderr, "Could not drop privileges temporarily to %d: %s\n", m_user, ::strerror(errno));
    return -1; // we are in the fork
  }
#ifdef DEBUG
  printf("Dropped into user %d\n", m_user);
#endif
  return 0;
#else
  return 1;
#endif
}
int Honeycomb::perm_drop() {
#ifdef HAS_SETRESGID
  uid_t ruid, euid, suid;
  gid_t rgid, egid, sgid;
  if (setresgid(m_user, m_user, m_user) || setresuid(m_user, m_user, m_user) || getresgid(&rgid, &egid, &sgid)
      || getresuid(&ruid, &euid, &suid) || rgid != m_user || egid != m_user || sgid != m_user
      || ruid != m_user || euid != m_user || suid != m_user || getegid() != m_user || geteuid() != m_user ) {
    fprintf(stderr,"\nError setting user to %u\n",m_user);
    return -1;
  }
  return 0;
#else
  return 1;
#endif
}
int Honeycomb::restore_perms() {
#ifdef HAS_SETRESGID
  uid_t ruid, euid, suid;
  gid_t rgid, egid, sgid;
  DEBUG_MSG("Recovering into '%d' user\n", (int)geteuid());

  if (getresgid(&rgid, &egid, &sgid) || getresuid(&ruid, &euid, &suid) || setresuid(-1, suid, -1) || setresgid(-1, sgid, -1)
      || geteuid() != suid || getegid() != sgid ) {
        fprintf(stderr, "Could not restore privileges to %d: %s\n", m_user, ::strerror(errno));
        return -1; // we are in the fork
      }
  return 0;
#else
  return 1;
#endif
}

/*------------------------ INTERNAL -------------------------*/
void Honeycomb::init() {
  // ei::Serializer m_eis(2);
  m_nofiles = NULL;
  std::stringstream stream;
  std::string token;
  
  // Setup defaults here
  m_user = INT_MAX;
  m_group = INT_MAX;
  
  // Run through the config
  if (m_honeycomb_config != NULL) {
    //--- User
    if (m_honeycomb_config->user != NULL) {
      struct passwd *pw = getpwnam(m_honeycomb_config->user);
      if (pw == NULL) {
        fprintf(stderr, "Error: invalid user %s : %s\n", m_honeycomb_config->user, ::strerror(errno));
        m_user = geteuid();
      } else m_user = pw->pw_uid;
    } else m_user = random_uid();
    
    //--- group
    if (m_honeycomb_config->group != NULL) {
      struct group *grp = getgrnam(m_honeycomb_config->group);
      if (grp == NULL) {
        fprintf(stderr, "Error: invalid group %s : %s\n", m_honeycomb_config->group, ::strerror(errno));
        m_group = getgid();
      }
      else m_group = grp->gr_gid;
    } else m_group = random_uid();
    
    //--- root_directory
    if (m_honeycomb_config->root_dir != NULL) set_root_dir(m_honeycomb_config->root_dir);
    //--- storage_dir
    if (m_honeycomb_config->storage_dir != NULL) m_storage_dir = m_honeycomb_config->storage_dir;
    //--- run_dir
    if (m_honeycomb_config->run_dir != NULL) m_run_dir = m_honeycomb_config->run_dir;
    
    //--- executables
    m_executables.insert("/bin/ls");
    m_executables.insert("/bin/bash");
    m_executables.insert("/usr/bin/whoami");
    m_executables.insert("/usr/bin/env");
    m_executables.insert(getenv("SHELL"));
    
    if (m_honeycomb_config->executables != NULL) {
      stream << m_honeycomb_config->executables; // Insert into a string
      while ( getline(stream, token, ' ') ) m_executables.insert(token);
    }
    
    //--- directories
    if (m_honeycomb_config->directories != NULL) {
      stream.clear(); token = "";
      stream << m_honeycomb_config->directories;
      while ( getline(stream, token, ' ') ) m_dirs.insert(token);
    }
    
    //--- other files
    if (m_honeycomb_config->files != NULL) {
      stream.clear(); token = "";
      stream << m_honeycomb_config->files;
      while ( getline(stream, token, ' ') ) m_files.insert(token);
    }
        
    //--- image
    if (m_honeycomb_config->image != NULL) m_image = m_honeycomb_config->image;
    //--- skel directory
    if (m_honeycomb_config->skel_dir != NULL) m_skel_dir = m_honeycomb_config->skel_dir;    
    
    //--- confinement_mode
    m_mode = S_IRWXU | S_IRGRP | S_IROTH; // Not sure if this should be dynamic-able, yet.
    
    //--- We need a name!
    if (m_name == "") m_name = to_string(random_uid(), 10);
  }
}

int Honeycomb::valid() {
  // Run validations on the honeycomb here
  return 0;
}