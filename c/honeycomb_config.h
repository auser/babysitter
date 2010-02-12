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
  T_DIRECTORIES,
  T_EXECUTABLES,
  T_ENV,
  T_STDOUT,
  T_STDIN,
  T_ROOT_DIR
} attr_type;

typedef struct _phase_ {
  phase_type type;
  char *before;
  char *command;
  char **command_array;
  int num_exec_lines;
  char *after;
} phase;

typedef struct _honeycomb_config_ {
  char *filename;           // Filename of the config file
  char *app_type;           // Application type (i.e. rack, java, etc.)
  char *root_directory;     // Root directory to operate inside of (can be generated)
  char *environment_vars;   // a list of environment variables to start
  char *stdout;             // STDOUT
  char *stdin;              // STDIN
  char **exec;              // Executable code
  size_t num_exec_lines;    // Executable lines
  // Phases
  size_t num_phases;
  phase **phases;
} honeycomb_config;

#endif