#ifndef HONEYCOMB_H
#define HONEYCOMB_H

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

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
typedef enum _phase_type_ {
  T_BUNDLE,
  T_START,
  T_STOP,
  T_MOUNT,
  T_UNMOUNT,
  T_CLEANUP,
  T_UNKNOWN
} phase_type;

typedef enum _attr_type_ {
  T_FILEPATH,
  T_DIRECTORIES,
  T_EXECUTABLES,
  T_FILES,
  T_ENV,
  T_STDOUT,
  T_STDIN,
  T_USER,
  T_GROUP,
  T_IMAGE,
  T_SKEL_DIR,
  T_RUN_DIR,
  T_ROOT_DIR
} attr_type;

typedef struct _phase_ {
  phase_type type;
  char *before;
  char *command;
  char *after;
} phase;

typedef struct _honeycomb_config_ {
  char *filepath;       // The filepath of the config file (TODO: so we can use relative filepaths)
  char *app_type;       // Application type (i.e. rack, java, etc.)
  char *root_dir;       // Root directory to operate inside of (can be generated)
  char *run_dir;        // Run directory to mount and run bees in
  char *env;            // a list of environment variables to start
  char *executables;    // Extra executables
  char *directories;    // Extra directories to copy over
  char *files;          // Extra files to copy
  char *stdout;         // STDOUT
  char *stdin;          // STDIN
  char *user;           // The user to run this honeycomb
  char *group;          // The group to run this honeycomb
  char *image;          // Image to mount as bee
  char *skel_dir;       // Skeleton directory to copy instead of building it
  // Phases
  size_t num_phases;
  phase **phases;
} honeycomb_config;

#endif