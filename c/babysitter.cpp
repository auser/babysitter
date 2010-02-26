/**
 * Babysitter -- Mount and run a command in a chrooted environment with mouted image support
 *
**/

// Core includes
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>

// STD
#include <string>
#include <map>

// Our own includes
#include "babysitter_utils.h"
#include "honeycomb_config.h"
#include "hc_support.h"
#include "babysitter.h"

// Globals
int alarm_max_time      = 12;
int to_set_user_id = -1;

std::deque<PidStatusT> exited_children;
sigjmp_buf  jbuf;
bool oktojump   = false;
bool pipe_valid = true;      // Is the pip still valid?
int  terminated = 0;         // indicates that we got a SIGINT / SIGTERM event
long int dbg     = 0;        // Debug flag
bool signaled   = false;     // indicates that SIGCHLD was signaled


// Configs
std::string config_file_dir;                // The directory containing configs
std::string root_dir;                       // The root directory to work from within
std::string run_dir;                        // The directory to run the bees
std::string working_dir;                    // Working directory
std::string storage_dir;                    // Storage dir
std::string sha;                            // The sha
std::string name;                           // Name
std::string scm_url;                        // The scm url
std::string image;                          // The image to mount
string_set  execs;                          // Executables to add
string_set  files;                          // Files to add
string_set  dirs;                           // Dirs to add
ConfigMapT  known_configs;                  // Map containing all the known application configurations (in /etc/beehive/apps)
phase_type  action;
int port;                                   // Port to run
char        app_type[BUF_SIZE];             // App type defaults to rack
char        usr_action_str[BUF_SIZE];       // Used for error printing

void version(FILE *fp)
{
  fprintf(fp, "babysitter version %1f\n", BABYSITTER_VERSION);
}

/**
 * Setup the signal handlers for *this* process
 **/
void setup_signal_handlers(struct sigaction sact, struct sigaction sterm)
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
  
  // Deque of all pids that exited and have their exit status available.
  exited_children.resize(SIGCHLD_MAX_SIZE);
}

// We've received a signal to process
void gotsignal(int sig) 
{
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
  
  debug(dbg, 1, "Process %d exited (sig=%d)\r\n", si->si_pid, signal);
  process_child_signal(si->si_pid);
  if (oktojump) siglongjmp(jbuf, 1);
}

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

/**
 * Check for the pending messages, so we don't miss any
 * child signal messages
 **/
void check_pending()
{
  sigset_t  set;
  siginfo_t info;
  sigemptyset(&set);
  if (sigpending(&set) == 0) {
    if (sigismember(&set, SIGCHLD))      gotsigchild(SIGCHLD, &info, NULL);
    else if (sigismember(&set, SIGPIPE)) {pipe_valid = false; gotsignal(SIGPIPE);}
    else if (sigismember(&set, SIGTERM)) gotsignal(SIGTERM);
    else if (sigismember(&set, SIGINT))  gotsignal(SIGINT);
    else if (sigismember(&set, SIGHUP))  gotsignal(SIGHUP);
  }
}


void usage(int c, bool detailed = false)
{
  FILE *fp = c ? stderr : stdout;
    
  version(fp);
  fprintf(fp, "Copyright 2010. Ari Lerner. Cloudteam @ AT&T interactive\n");
  fprintf(fp, "This program may be distributed under the terms of the MIT License.\n\n");
  fprintf(fp, "Usage: babysitter <command> [options]\n"
  "babysitter bundle | mount | start | stop | unmount | cleanup\n"
  "* Options\n"
  "*  --help [detailed]   | -h [d]          Show this message. [detailed|d] will generate more information.\n"
  "*  --storage_dir <dir> | -b <dir>        Hive directory to store the sleeping bees\n"
  "*  --config <dir>      | -c <dir>        The directory or file containing the config files (default: /etc/beehive/config)\n"
  "*  --dir <dir>         | -d <dir>        The directory\n"
  "*  --debug             | -D <level>      Turn on debugging flag\n"
  "*  --exec <exec>       | -e <exec>       Add an executable to the paths\n"
  "*  --file <file>       | -f <file>       Add this file to the path\n"
  "*  --image <file>      | -i <file>       The image to mount that contains the bee\n"
  "*  --scm_url <url>     | -m <url>        The scm_url\n"
  "*  --name <name>       | -n <name>       Name of the app\n"
  "*  --root <dir>        | -o <dir>        Base directory (default: /var/beehive)\n"
  "*  --port <port>       | -p <port>       The port\n"
  "*  --run_dir <dir>     | -r <dir>        The directory to run the bees (default: $ROOT_DIR/active)\n"
  "*  --sha <sha>         | -s <sha>        The sha of the bee\n"
  "*  --type <type>       | -t <type>       The type of application (defaults to rack)\n"
  "*  --user <user>       | -u <user>       User to run as\n"
  "*  --working <dir>     | -w <dir>        Working directory (default: $ROOT_DIR/scratch)\n"
  "\n"
  );
  
  if (detailed)
    fprintf(fp, "Usage: babysitter <command> [options]\n"
      "Babysitter commands:\n"
      "\tbundle   - This bundles the bee into a single file\n"
      "\tmount    - This is responsible for actually mounting the bee\n"
      "\tstart    - This will call the start command action on the bee\n"
      "\tstop     - This will call the stop command action on the bee\n"
      "\tunmount  - This will call the unmount command on the bee\n"
      "\tcleanup  - This will force the cleanup action on the bee\n"
      "\n"
      "All of the actions are described in configuration files. By passing the --config <dir> | -c <dir>\n"
      "switches, you can override the default configuration file directory location.\n"
      "The bees are mapped to their respective configuration file name by the type of bee that's launched\n"
      "\n"
      "The bee's configuration file sets up actions on how to handle the different actions and hooks the\n"
      "actions will call. For example, to control bundling, an action might look like this:\n\n"
      "\tbundle: {\n"
      "\t mkdir -p $STORAGE_DIR\n"
      "\t mkdir -p `dirname $BEE_IMAGE`\n"
      "\t tar czf $BEE_IMAGE ./\n"
      "\t}\n"
      "\tbundle.after: mail -s 'Bundled' root@localhost\n\n"
      "These actions will be called when babysitter calls the action.\n"
      "There are 5 actions that can be overridden from their defaults from within babysitter.These are\n"
      "bundle, mount, start, stop, unmount and cleanup\n"
      "The environment variables that are available to the bee are as follows:\n"
      "\tBEE_IMAGE    - The location of the compressed bee (or locaton or the compressed bee, when 'bundle' is called)\n"
      "\tAPP_NAME     - The name of the application\n"
      "\tRUN_DIR      - The directory to run a bee from within\n"
      "\tAPP_TYPE     - The application type\n"
      "\tBEE_SIZE     - The size of the bee, in Kb\n"
      "\tAPP_USER     - The ephemeral user id of the operation\n"
      "\tBEE_PORT     - The port to run the bee on\n"
      "\tSCM_URL      - The location of the scm to check out from (useful for 'bundle' action)\n"
      "\tFILESYSTEM   - The preferred filesystem type\n"
      "\tWORKING_DIR  - The directory to work from within\n"
      "\tSTORAGE_DIR  - The directory to store the final bees in\n"
      "\t\n"
      "\n"
      );
      
  exit(c);
}

void setup_defaults() {
  to_set_user_id = -1;
  config_file_dir = "/etc/beehive/config";
  action = T_EMPTY;
  memset(app_type, 0, BUF_SIZE);
  strncpy(app_type, "rack", 4);
}

/**
 * Relatively inefficient command-line parsing, but... 
 * this isn't speed-critical, so it doesn't matter
**/
void parse_the_command_line(int argc, char *argv[])
{
  char *opt;
  while (argc > 1) {
    opt = argv[1];
    // OPTIONS
    if (!strncmp(opt, "--debug", 7) || !strncmp(opt, "-D", 2)) {
      if (argv[2] == NULL) {
        fprintf(stderr, "You must pass a level with the debug flag\n");
        usage(1);
      }
      char * pEnd;
      dbg = strtol(argv[2], &pEnd, 10);
      argc--; argv++;
    } else if (!strncmp(opt, "--help", 6) || !strncmp(opt, "-h", 2)) {
      if(argv[2] != NULL && (!strncmp(argv[2], "d", 1) || !strncmp(argv[2], "detailed", 8)))
        usage(0, true);
      else 
        usage(0, false);
    } else if (!strncmp(opt, "--name", 6) || !strncmp(opt, "-n", 2)) {
      name = argv[2];
      argc--; argv++;
    } else if (!strncmp(opt, "--port", 6) || !strncmp(opt, "-p", 2)) {
      port = atoi(argv[2]);
      argc--; argv++;
    } else if (!strncmp(opt, "--run_dir", 9) || !strncmp(opt, "-r", 2)) {
      run_dir = argv[2];
      argc--; argv++;
    } else if (!strncmp(opt, "--type", 6) || !strncmp(opt, "-t", 2)) {
      memset(app_type, 0, BUF_SIZE);
      strncpy(app_type, argv[2], strlen(argv[2]));
      argc--; argv++;
    } else if (!strncmp(opt, "--image", 7) || !strncmp(opt, "-i", 2)) {
      image = argv[2];
      argc--; argv++;
    } else if (!strncmp(opt, "--storage_dir", 10) || !strncmp(opt, "-b", 2)) {
      storage_dir = argv[2];
      argc--; argv++;
    } else if (!strncmp(opt, "--sha", 6) || !strncmp(opt, "-s", 2)) {
      sha = argv[2];
      argc--; argv++;
    } else if (!strncmp(opt, "--exec", 6) || !strncmp(opt, "-e", 2)) {
      execs.insert(argv[2]);
      argc--; argv++;      
    } else if (!strncmp(opt, "--file", 6) || !strncmp(opt, "-f", 2)) {
      files.insert(argv[2]);
      argc--; argv++;
    } else if (!strncmp(opt, "--dir", 6) || !strncmp(opt, "-d", 2)) {
      dirs.insert(argv[2]);
      argc--; argv++;
    } else if (!strncmp(opt, "--scm_url", 9) || !strncmp(opt, "-m", 2)) {
      scm_url = argv[2];
      argc--; argv++;
    } else if (!strncmp(opt, "--config", 8) || !strncmp(opt, "-c", 2)) {
      config_file_dir = argv[2];
      argc--; argv++;
    } else if (!strncmp(opt, "--root", 6) || !strncmp(opt, "-o", 2)) {
      root_dir = argv[2];
      argc--; argv++;
    } else if (!strncmp(opt, "--working_dir", 13) || !strncmp(opt, "-w", 2)) {
      working_dir = argv[2];
      argc--; argv++;
    } else if (!strncmp(opt, "--user", 6) || !strncmp(opt, "-u", 2)) {
      struct passwd *pw;
      if ((pw = getpwnam(argv[2])) == 0) {
        to_set_user_id = (uid_t)pw->pw_uid;
      } else {
        fprintf(stderr, "Could not get name for user: %s: %s\n", argv[2], ::strerror(errno));
      }
      argc--; argv++;
    // ACTIONS
    } else if (!strncmp(opt, "bundle", 6)) {
      action = T_BUNDLE;
    } else if (!strncmp(opt, "start", 5)) {
      action = T_START;
    } else if (!strncmp(opt, "stop", 4)) {
      action = T_STOP;
    } else if (!strncmp(opt, "mount", 5)) {
      action = T_MOUNT;
    } else if (!strncmp(opt, "unmount", 7)) {
      action = T_UNMOUNT;
    } else if (!strncmp(opt, "cleanup", 7)) {
      action = T_CLEANUP;
    } else {
      if(action == T_EMPTY)
      {
        action = T_UNKNOWN;
        memset(usr_action_str, 0, BUF_SIZE);
        strncpy(usr_action_str, opt, strlen(opt));
      } else {
        fprintf(stderr, "Unknown switch: %s. Try passing --help for help options\n", opt);
        usage(1);
      }
    }
    argc--; argv++;
  }
}


int main (int argc, char *argv[])
{
  setup_defaults();
  parse_the_command_line(argc, argv);

  if (action == T_UNKNOWN) {
    fprintf(stderr, "Unknown action: %s\n", usr_action_str);
    usage(1);
  } else if (action == T_EMPTY) {
    fprintf(stderr, "No action defined. Babysitter won't do anything if you don't tell it to do something.\n");
    usage(1);
  }
  char *action_str = phase_type_to_string(action);
  parse_config_dir(config_file_dir, known_configs);
  
  debug(dbg, 1, "--- running action: %s ---\n", action_str);
  debug(dbg, 1, "\tapp type: %s\n", app_type);
  debug(dbg, 1, "\troot dir: %s\n", root_dir.c_str());
  debug(dbg, 1, "\tsha: %s\n", sha.c_str());
  debug(dbg, 1, "\tconfig dir: %s\n", config_file_dir.c_str());
  debug(dbg, 1, "\tuser id: %d\n", to_set_user_id);
  debug(dbg, 1, "\trun dir: %d\n", run_dir.c_str());
  if (dbg > 1) {
    printf("--- files ---\n"); for(string_set::iterator it=files.begin(); it != files.end(); it++) printf("\t - %s\n", it->c_str());
    printf("--- dirs ---\n"); for(string_set::iterator it=dirs.begin(); it != dirs.end(); it++) printf("\t - %s\n", it->c_str());
    printf("--- execs ---\n"); for(string_set::iterator it=execs.begin(); it != execs.end(); it++) printf("\t - %s\n", it->c_str());
  }
  debug(dbg, 1, "\tnumber of configs in config directory: %d\n", (int)known_configs.size());
  debug(dbg, 1, "--- ---\n");
  
  if (dbg > 3) {
    for(ConfigMapT::iterator it=known_configs.begin(), end=known_configs.end(); it != end; ++it) {
      std::string f = it->first;
      honeycomb_config *c = it->second;
      printf("------ %s ------\n", c->filepath);
      if (c->directories != NULL) printf("directories: %s\n", c->directories);
      if (c->executables != NULL) printf("executables: %s\n", c->executables);

      printf("------ phases (%d) ------\n", (int)c->num_phases);
      unsigned int i;
      for (i = 0; i < (unsigned int)c->num_phases; i++) {
        printf("Phase: --- %s ---\n", phase_type_to_string(c->phases[i]->type));
        if (c->phases[i]->before != NULL) printf("Before -> %s\n", c->phases[i]->before);
        printf("Command -> %s\n", c->phases[i]->command);
        if (c->phases[i]->after != NULL) printf("After -> %s\n", c->phases[i]->after);
        printf("\n");
      }
    }
  }
  
  // Honeycomb
  ConfigMapT::iterator it;
  if (known_configs.count(app_type) < 1) {
    fprintf(stderr, "There is no config file set for this application type.\nPlease set the application type properly, or consult the administrator to support the application type: %s\n", app_type);
    usage(1);
  }
  debug(dbg, 2, "\tconfig for %s app found\n", app_type);
  
  it = known_configs.find(app_type);
  honeycomb_config *c = it->second;
  Honeycomb comb (app_type, c);
  
  for(string_set::iterator it=files.begin(); it != files.end(); it++) comb.add_file(it->c_str());
  for(string_set::iterator it=dirs.begin(); it != dirs.end(); it++) comb.add_dir(it->c_str());
  for(string_set::iterator it=execs.begin(); it != execs.end(); it++) comb.add_executable(it->c_str());
  
  if (name != "") comb.set_name(name);
  if (image != "") comb.set_image(image);
  if (to_set_user_id != -1) comb.set_user(to_set_user_id);
  if (sha != "") comb.set_sha(sha);
  if (working_dir != "") comb.set_working_dir(working_dir);
  if (root_dir != "") comb.set_root_dir(root_dir);
  if (scm_url != "") comb.set_scm_url(scm_url);
  if (port != -1) comb.set_port(port);
  if (storage_dir != "") comb.set_storage_dir(storage_dir);
  if (run_dir != "") comb.set_run_dir(run_dir);
  
  switch(action) {
    case T_BUNDLE: 
      comb.bundle(dbg); break;
    case T_START:
      comb.start(dbg); break;
    case T_STOP:
      comb.stop(dbg); break;
    case T_MOUNT:
      comb.mount(dbg); break;
    case T_UNMOUNT:
      comb.unmount(dbg); break;
    case T_CLEANUP:
      comb.cleanup(dbg); break;
    case T_EMPTY:
    case T_UNKNOWN:
    break;
  }
  
  for(ConfigMapT::iterator it=known_configs.begin(), end=known_configs.end(); it != end; ++it) {
    debug(dbg, 4, "Freeing: %s\n", ((std::string) it->first).c_str());
    free_config((honeycomb_config *) it->second);
  }
  
  return 0;
}