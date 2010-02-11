#include <stdio.h>
#include <stdlib.h>

#include "honeycomb_config.h"
#include "c_ext.h"

int debug_level = 3;
#define BUF_SIZE 1024

int debug(int level, char *fmt, ...) {
  int r;
  va_list ap;
  if (debug_level < level) return 0;
	va_start(ap, fmt);
	r = vfprintf(stderr, fmt, ap);
	va_end(ap);
	return r;
}

/**
* turn a phase_type into a string
**/
char *ptype_to_string(phase_type t) {
  switch (t) {
    case T_BUNDLE:
      return strdup("bundle");
      break;
    case T_START:
      return strdup("start");
      break;
    case T_STOP:
      return strdup("stop");
      break;
    case T_MOUNT:
      return strdup("mount");
      break;
    case T_UNMOUNT:
      return strdup("unmount");
      break;
    case T_CLEANUP:
      return strdup("cleanup");
      break;
    case T_UNKNOWN:
    default:
      return strdup("unknown");
      break;
  }
}

/**
* Split a string and collect the characters until the '.'
*   bundle.before -> bundle
**/
char *collect_to_period(char *str) {
  char buf[BUF_SIZE];
  int i = 0;
  /* strip off the .before (gross) */
  memset(buf, 0, BUF_SIZE);
  for (i = 0; i < strlen(str); i++) {
    if (str[i] == '.') {
      buf[i + 1] = 0;
      break;
    }
    buf[i] = str[i];
  }
  return strdup(buf);
}

// create a new config
honeycomb_config* new_config() {
  honeycomb_config *c;
	c = malloc(sizeof(honeycomb_config *));
	if (c) {
    return c;
	} else {
    fprintf(stderr, "Error creating a new config object\n");
    return NULL;
	}
	// Should never get here
}

// Create a new phase
phase* new_phase(phase_type t) {
  phase *p;
  p = malloc(sizeof(phase *));
  if (p) {
    debug(3, "Created a new phase: %d\n", t);
    p->type = t;
    p->before = 0;
    p->command = 0;
    p->after = 0;
  } else {
  }
  return p;
}

// Add a phase to the honeycomb_config
int add_phase(honeycomb_config *c, phase *p) {
  int n = c->num_phases + 1;
  phase **nphases = malloc(sizeof(phase *) * n);
  
  if (!nphases) {
    fprintf(stderr, "Couldn't add phase: '%d', no memory available\n", p->type);
    return -1;
  }
  int i;
  for (i = 0; i < c->num_phases; ++i) {
    nphases[i] = c->phases[i];
  }
  
  free(c->phases);
  c->phases = nphases;
  c->num_phases = n;
  
  c->phases[c->num_phases++] = p;
  return 0;
}

// Free a config object
void free_config(honeycomb_config *c) {
  if (!c) return;
  size_t i;
  for (i = 0; i < c->num_phases; i++) {
    free_phase(c->phases[i]);
  }
  free(c->filename);
  free(c->app_type);
  free(c->root_directory);
  free(c->environment_vars);
  free(c->stdout);
  free(c->stdin);
  for(i = 0; i < c->num_exec_lines; ++i) free(c->exec[i]);
  free(c->exec);
}

// Ffree a phase struct
void free_phase(phase *p) {
  if (!p) return;
  free(p->before);
  free(p->command);
  free(p->after);
}
