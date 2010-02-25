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

int alarm_max_time      = 12;
static long int dbg     = 0;
int userid = -1;

// Configs
std::string config_file_dir;                // The directory containing configs
std::string root_dir;                       // The root directory to work from within
std::string run_dir;                        // The directory to run the bees
std::string hive_dir;                       // Hive dir
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

void usage(int c)
{
  FILE *fp = c ? stderr : stdout;
    
  version(fp);
  fprintf(fp, "Copyright 2010. Ari Lerner. Cloudteam @ AT&T interactive\n");
  fprintf(fp, "This program may be distributed under the terms of the MIT License.\n\n");
  fprintf(fp, "Usage: babysitter <command> [options]\n"
  "babysitter bundle | mount | start | stop | unmount | cleanup\n"
  "* Options\n"
  "*  --help            | -h              Show this message\n"
  "*  --debug           | -D <level>      Turn on debugging flag'\n"
  "*  --port <port>     | -p <port>       The port to run on"
  "*  --hive_dir <dir>  | -b <dir>        Hive directory to store the sleeping bees\n"
  "*  --name <name>     | -n <name>       Name of the app\n"
  "*  --user <user>     | -u <user>       User to run as\n"
  "*  --type <type>     | -t <type>       The type of application (defaults to rack)\n"
  "*  --root <dir>      | -r <dir>        The directory where the bees will be created\n"
  "*  --run_dir <dir>   | -l <dir>        The directory to run the bees"
  "*  --sha <sha>       | -s <sha>        The sha of the bee\n"
  "*  --image <file>    | -i <file>       The image to mount that contains the bee\n"
  "*  --exec <exec>     | -e <exec>       Add an executable to the paths\n"
  "*  --file <file>     | -f <file>       Add this file to the path\n"
  "*  --dir <dir>       | -d <dir>        The directory\n"
  "*  --scm_url <url>   | -u <url>        The scm_url\n"
  "*  --config <dir>    | -c <dir>        The directory or file containing the config files\n\n"
  );
  
  exit(c);
}

void setup_defaults() {
  userid = -1;
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
      usage(0);
    } else if (!strncmp(opt, "--name", 6) || !strncmp(opt, "-n", 2)) {
      name = argv[2];
      argc--; argv++;
    } else if (!strncmp(opt, "--port", 6) || !strncmp(opt, "-p", 2)) {
      port = atoi(argv[2]);
      argc--; argv++;
    } else if (!strncmp(opt, "--run_dir", 9) || !strncmp(opt, "-l", 2)) {
      run_dir = argv[2];
      argc--; argv++;
    } else if (!strncmp(opt, "--type", 6) || !strncmp(opt, "-t", 2)) {
      memset(app_type, 0, BUF_SIZE);
      strncpy(app_type, argv[2], strlen(argv[2]));
      argc--; argv++;
    } else if (!strncmp(opt, "--image", 7) || !strncmp(opt, "-i", 2)) {
      image = argv[2];
      argc--; argv++;
    } else if (!strncmp(opt, "--hive_dir", 10) || !strncmp(opt, "-b", 2)) {
      hive_dir = argv[2];
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
    } else if (!strncmp(opt, "--root", 6) || !strncmp(opt, "-r", 2)) {
      root_dir = argv[2];
      argc--; argv++;
    } else if (!strncmp(opt, "--user", 6) || !strncmp(opt, "-u", 2)) {
      char* run_as_user = argv[2];
      struct passwd *pw = NULL;
      if ((pw = getpwnam(run_as_user)) == NULL) {
        fprintf(stderr, "User %s not found!\n", run_as_user);
        exit(3);
      }
      userid = pw->pw_uid;
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
  debug(dbg, 1, "\tuser id: %d\n", userid);
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
  if (userid != -1) comb.set_user(userid);
  if (sha != "") comb.set_sha(sha);
  if (root_dir != "") comb.set_root_dir(root_dir);
  if (scm_url != "") comb.set_scm_url(scm_url);
  if (port != -1) comb.set_port(port);
  if (hive_dir != "") comb.set_hive_dir(hive_dir);
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