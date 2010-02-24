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
#include "honeycomb_config.h"
#include "hc_support.h"
#include "babysitter.h"

int alarm_max_time      = 12;
static long int dbg     = 0;
int userid = 0;

// Configs
std::string config_file_dir;
ConfigMapT   known_configs;                 // Map containing all the known application configurations (in /etc/beehive/apps)
phase_type  action;
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
  );
  
  exit(c);
}

void setup_defaults() {
  userid = 0;
  config_file_dir = "/etc/beehive/config";
  action = T_EMPTY;
  memset(app_type, 0, BUF_SIZE);
  strncpy(app_type, "rack", 4);
}

/**
 * Relatively inefficient command-line parsing, but... 
 * this isn't speed-critical, so it doesn't matter
 *
 * Options
 *  --debug|-D <level>          Turn on debugging flag'
 *  --user <user> | -u <user>   User to run as
 *  --type <type> | -t <type>   The type of application (defaults to rack)
 *  --config <dir> | -c <dir>   The directory or file containing the config files
 *  action                      Action to run
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
    } else if (!strncmp(opt, "--type", 6) || !strncmp(opt, "-t", 2)) {
      memset(app_type, 0, BUF_SIZE);
      strncpy(app_type, argv[2], strlen(argv[2]));
      argc--; argv++;
    } else if (!strncmp(opt, "--config", 8) || !strncmp(opt, "-c", 2)) {
      config_file_dir = argv[2];
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
      action = T_UNKNOWN;
      memset(usr_action_str, 0, BUF_SIZE);
      strncpy(usr_action_str, opt, strlen(opt));
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
  debug(dbg, 1, "\tconfig dir: %s\n", config_file_dir.c_str());
  debug(dbg, 1, "\tuser id: %d\n", userid);
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
  
  switch(action) {
    case T_BUNDLE: 
      comb.bundle(dbg); break;
    case T_START:
      comb.start(); break;
    case T_STOP:
      comb.stop(); break;
    case T_MOUNT:
      comb.mount(); break;
    case T_UNMOUNT:
      comb.unmount(); break;
    case T_CLEANUP:
      comb.cleanup(); break;
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