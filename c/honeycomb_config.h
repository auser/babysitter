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

typedef enum phases {
  PHASE_BUNDLE,
  PHASE_MOUNT
} phase_type;

typedef struct _phase_ {
  phase_type phase;
  char *command;
} phase;

typedef struct _honeycomb_config_ {
  char *name;
  
} honeycomb_config;


#endif