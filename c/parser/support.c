#include <stdio.h>
#include <stdlib.h>

#include "honeycomb_config.h"
#include "c_ext.h"

int debug_level = 3;

int debug(int level, char *fmt, ...) {
  int r;
  va_list ap;
  if (debug_level < level) return 0;
	va_start(ap, fmt);
	r = vfprintf(stderr, fmt, ap);
	va_end(ap);
	return r;
}

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

phase* new_phase(char *name) {
  phase *p;
  p = malloc(sizeof(phase *));
  if (p) {
    debug(3, "Created a new phase: %s\n", name);
    p->name = name;
    p->before = 0;
    p->command = 0;
    p->after = 0;
  } else {
    free(name);
  }
  return p;
}

// Add a phase to the honeycomb_config
int add_phase(honeycomb_config *c, phase *p) {
  int n = c->num_phases + 1;
  phase **nphases = malloc(sizeof(phase *) * n);
  
  if (!nphases) {
    char *s = 0;
    if (p && p->name) strncpy(s, p->name, strlen(p->name));
    fprintf(stderr, "Couldn't add phase: '%s', no memory available\n", s ? s : "<null>");
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
  int i;
  for (i = 0; i < c->num_phases; i++) {
    free_phase(c->phases[i]);
  }
  free(c->filename);
  free(c->app_type);
  free(c->root_directory);
  free(c->environment_vars);
  free(c->stdout);
  free(c->stdin);
}

// Ffree a phase struct
void free_phase(phase *p) {
  if (!p) return;
  free(p->before);
  free(p->command);
  free(p->after);
}
