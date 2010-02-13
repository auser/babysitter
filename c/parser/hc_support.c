#include <stdio.h>
#include <stdlib.h>

#include "hc_support.h"

#define BUF_SIZE 1024

int debug(int level, char *fmt, ...) {
  int r;
  va_list ap;
  if (DEBUG_LEVEL < level) return 0;
	va_start(ap, fmt);
	r = vfprintf(stderr, fmt, ap);
	va_end(ap);
	return r;
}

/**
* turn a phase_type into a string
* FOR DEBUGGING
**/
char *phase_type_to_string(phase_type t) {
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

char *attribute_type_to_string(attr_type t) {
  switch (t) {
    case T_DIRECTORIES: return strdup("directories"); break;
    case T_EXECUTABLES: return strdup("executables"); break;
    case T_ENV: return strdup("env"); break;
    case T_STDOUT: return strdup("stdout"); break;
    case T_STDIN: return strdup("stdin"); break;
    case T_ROOT_DIR: return strdup("root_dir"); break;
    default: return strdup("unknown");
  }
}

phase_type str_to_phase_type(char *str) {
  if      (strcmp(str,"bundle") == 0) return T_BUNDLE;
  else if (strcmp(str,"start") == 0) return T_START;
  else if (strcmp(str,"stop") == 0) return T_STOP;
  else if (strcmp(str,"mount") == 0) return T_MOUNT;
  else if (strcmp(str,"unmount") == 0) return T_UNMOUNT;
  else if (strcmp(str,"cleanup") == 0) return T_CLEANUP;
  return -1;
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
honeycomb_config* a_new_honeycomb_config_object(void) {
  honeycomb_config *c = malloc(sizeof(honeycomb_config));
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

// Find a phase of type t on the config object
phase *find_phase(honeycomb_config *c, phase_type t) {
  int i;
  for (i = 0; i < c->num_phases; ++i) {
    if (c->phases[i]->type == t) return c->phases[i];
  }
  return NULL;
}

// Find or create a phase on a config object
phase *find_or_create_phase(honeycomb_config *c, phase_type t) {
  phase *p;
  if ((p = find_phase(c, t))) return p;
  return new_phase(t);
}

// Modify an existing phase
int modify_phase(honeycomb_config *c, phase *p) {
  int i;
  for (i = 0; i < c->num_phases; ++i) {
    if (c->phases[i]->type == p->type) {
      c->phases[i] = p;
      return 0;
    }
  }
  return -1;
}

// Add a phase to the honeycomb_config
int add_phase(honeycomb_config *c, phase *p) {
  phase *existing_phase;
  if ((existing_phase = find_phase(c, p->type))) return modify_phase(c, p);
  int n = c->num_phases + 1;
  phase **nphases = (phase **)malloc(sizeof(phase *) * n);
  
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
  
  c->phases[c->num_phases++] = p;
  return 0;
}

// Add an attribute to the config
// To add another, add it here, add it to the type and specify it in the
// honeycomb_config struct
int add_attribute(honeycomb_config *c, attr_type t, char *value) {
  switch (t) {
    case T_DIRECTORIES:
      c->directories = (char *)malloc(sizeof(char *) * strlen(value));
      c->directories = strdup(value); break;
    case T_EXECUTABLES:
      c->executables = (char *)malloc(sizeof(char *) * strlen(value));
      c->executables = strdup(value); break;
    case T_ENV:
      c->env = (char *)malloc(sizeof(char *) * strlen(value));
      c->env = strdup(value); break;
    case T_STDOUT:
      c->stdout = (char *)malloc(sizeof(char *) * strlen(value));
      c->stdout = strdup(value); break;
    case T_STDIN:
      c->stdin = (char *)malloc(sizeof(char *) * strlen(value));
      c->stdin = strdup(value); break;
    case T_ROOT_DIR:
      c->root_dir = (char *)malloc(sizeof(char *) * strlen(value));
      c->root_dir = strdup(value); break;
    case T_USER:
      c->user = (char *)malloc(sizeof(char *) * strlen(value));
      c->user = strdup(value); break;
    case T_GROUP:
      c->group = (char *)malloc(sizeof(char *) * strlen(value));
      c->group = strdup(value); break;
    case T_IMAGE:
      c->image = (char *)malloc(sizeof(char *) * strlen(value));
      c->image = strdup(value); break;
    default:
      fprintf(stderr, "Unknown attribute: %s = %s on config\n", attribute_type_to_string(t), value);
      return -1;
  }
  return 0;
}

// Free a config object
void free_config(honeycomb_config *c) {
  size_t i;
  for (i = 0; i < c->num_phases; i++) {
    if (c->phases[i]) {
      free_phase(c->phases[i]);
    }
  }
  free(c->filename);
  free(c->app_type);
  free(c->root_dir);
  free(c->env);
  free(c->stdout);
  free(c->stdin);
}

// Free a phase struct
void free_phase(phase *p) {
  if (!p) return;
  if (p->before) free(p->before);
  if (p->command) free(p->command);
  if (p->after) free(p->after);
}
