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
typedef struct _phase_ {
  char *name;
  char *before;
  char *command;
  char *after;
} phase;

typedef struct _honeycomb_config_ {
  char *filename;           // Filename of the config file
  char *app_type;           // Application type (i.e. rack, java, etc.)
  char *root_directory;     // Root directory to operate inside of (can be generated)
  char *environment_vars;   // a list of environment variables to start
  char *stdout;             // STDOUT
  char *stdin;              // STDIN
  // Phases
  size_t num_phases;
  phase **phases;
} honeycomb_config;

#endif