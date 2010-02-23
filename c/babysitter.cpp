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

int alarm_max_time     = 12;
static bool dbg        = false;
int userid = 0;

// Configs
std::string config_file_dir;
extern FILE *yyin;
ConfigMapT   known_configs;         // Map containing all the known application configurations (in /etc/beehive/apps, unless
phase_type  action;
char        usr_action_str[BUF_SIZE];           // Used for error printing

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
}

/**
 * Relatively inefficient command-line parsing, but... 
 * this isn't speed-critical, so it doesn't matter
 *
 * Options
 *  --debug|-D                  Turn on debugging flag'
 *  --user <user> | -u <user>   User to run as
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
      dbg = true;
      printf("Debugging!\n");
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
      strncpy(usr_action_str, opt, strlen(opt));
    }
    argc--; argv++;
  }
}


int main (int argc, char *argv[])
{
  parse_the_command_line(argc, argv);
  printf("In babysitter!\n");
  if (action == T_UNKNOWN) {
    fprintf(stderr, "Unknown action: %s\n", usr_action_str);
    usage(1);
  }
  char *action_str = phase_type_to_string(action);
  parse_config_dir(config_file_dir, known_configs);
  
  printf("--- running action: %s ---\n", action_str);
  printf("\tconfig dir: %s\n", config_file_dir.c_str());
  printf("\tuser id: %d\n", userid);
  
  printf("\tnumber of configs: %d\n", (int)known_configs.size());
  return 0;
}