#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "hc_support.h"
#include "honeycomb.h"
#include "honeycomb_config.h"
#include "print_utils.h"
#include "babysitter_types.h"

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
  if      (strncmp(str,"bundle", 6) == 0) return T_BUNDLE;
  else if (strncmp(str,"start", 5) == 0) return T_START;
  else if (strncmp(str,"stop", 4) == 0) return T_STOP;
  else if (strncmp(str,"mount", 5) == 0) return T_MOUNT;
  else if (strncmp(str,"unmount", 7) == 0) return T_UNMOUNT;
  else if (strncmp(str,"cleanup", 7) == 0) return T_CLEANUP;
  return T_UNKNOWN;
}

/**
* Split a string and collect the characters until the '.'
*   bundle.before -> bundle
**/
char *collect_to_period(char *str) {
  char buf[BUF_SIZE];
  unsigned int i = 0;
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
  honeycomb_config* c;
  
  if ( (c = (honeycomb_config *) malloc(sizeof(honeycomb_config))) == NULL ) {
    fprintf(stderr, "Could not allocate a new honeycomb_config object. Out of memory\n");
    exit(-1);
  }
  c->num_phases = 0;
  c->phases = NULL;
  
  c->app_type = NULL;
  c->root_dir = NULL;
  c->run_dir = NULL;
  c->env = NULL;
  c->executables = NULL;
  c->directories = NULL;
  c->files = NULL;
  c->clone_command = NULL;
  c->stdout = NULL;
  c->stdin = NULL;
  c->user = NULL;
  c->group = NULL;
  c->image = NULL;
  c->skel_dir = NULL;
  c->storage_dir = NULL;
  
  return c;
	// Should never get here
}

// Create a new phase
phase* new_phase(phase_type t) {
  phase *p;
  p = (phase*)malloc(sizeof(phase));
  if (p) {
    p->type = t;
    p->before = NULL;
    p->command = NULL;
    p->after = NULL;
  } else {
  }
  assert(p != NULL);
  return p;
}

// Find a phase of type t on the config object
phase *find_phase(honeycomb_config *c, phase_type t, int dlvl)
{
  unsigned int i = 0;
  for (i = 0; i < c->num_phases; i++) {
    if (c->phases[i]->type == t) {
      return c->phases[i];
    }
  }
  return NULL;
}

// Find or create a phase on a config object
phase *find_or_create_phase(honeycomb_config *c, phase_type t) {
  phase *p;
  if ((p = find_phase(c, t, 0))) return p;
  return new_phase(t);
}

// Modify an existing phase
int modify_phase(honeycomb_config *c, phase *p) {
  unsigned int i;
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
  if ((existing_phase = find_phase(c, p->type, 0))) return modify_phase(c, p);
  int n = c->num_phases + 1;
  phase **nphases = (phase **)malloc(sizeof(phase) * n);
  
  if (!nphases) {
    fprintf(stderr, "Couldn't add phase: '%d', no memory available\n", p->type);
    return -1;
  }
  
  unsigned int i;
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
int add_attribute(honeycomb_config *c, attr_type t, char *value)
{
  switch (t) {
    case T_FILEPATH:
      c->filepath = (char *) malloc(sizeof(char*) * strlen(value));
      memset(c->filepath, 0, strlen(value)); memcpy(c->filepath, value, strlen(value)); break;
    case T_DIRECTORIES:
      c->directories = (char *)malloc(sizeof(char *) * strlen(value));
      memset(c->directories, 0, strlen(value)); memcpy(c->directories, value, strlen(value)); break;
    case T_EXECUTABLES:
      c->executables = (char *)malloc(sizeof(char *) * strlen(value));
      memset(c->executables, 0, strlen(value)); memcpy(c->executables, value, strlen(value)); break;
    case T_FILES:
      c->files = (char *)malloc(sizeof(char *) * strlen(value));
      memset(c->files, 0, strlen(value)); memcpy(c->files, value, strlen(value)); break;
    case T_ENV:
      c->env = (char *)malloc(sizeof(char *) * strlen(value));
      memset(c->env, 0, strlen(value)); memcpy(c->env, value, strlen(value)); break;
    case T_STDOUT:
      c->stdout = (char *)malloc(sizeof(char *) * strlen(value));
      memset(c->stdout, 0, strlen(value)); memcpy(c->stdout, value, strlen(value)); break;
    case T_STDIN:
      c->stdin = (char *)malloc(sizeof(char *) * strlen(value));
      memset(c->stdin, 0, strlen(value)); memcpy(c->stdin, value, strlen(value)); break;
    case T_ROOT_DIR:
      c->root_dir = (char *)malloc(sizeof(char *) * strlen(value));
      memset(c->root_dir, 0, strlen(value)); memcpy(c->root_dir, value, strlen(value)); break;
    case T_RUN_DIR:
      c->run_dir = (char *)malloc(sizeof(char *) * strlen(value));
      memset(c->run_dir, 0, strlen(value)); memcpy(c->run_dir, value, strlen(value)); break;
    case T_USER:
      c->user = (char *)malloc(sizeof(char *) * strlen(value));
      memset(c->user, 0, strlen(value)); memcpy(c->user, value, strlen(value)); break;
    case T_GROUP:
      c->group = (char *)malloc(sizeof(char *) * strlen(value));
      memset(c->group, 0, strlen(value)); memcpy(c->group, value, strlen(value)); break;
    case T_CLONE_CMD:
      c->clone_command = (char *)malloc(sizeof(char *) * strlen(value));
      memset(c->clone_command, 0, strlen(value)); memcpy(c->clone_command, value, strlen(value)); break;
    case T_IMAGE:
      c->image = (char *)malloc(sizeof(char *) * strlen(value));
      memset(c->image, 0, strlen(value)); memcpy(c->image, value, strlen(value)); break;
    case T_STORAGE_DIR:
      c->storage_dir = (char *)malloc(sizeof(char *) * strlen(value));
      memset(c->storage_dir, 0, strlen(value)); memcpy(c->storage_dir, value, strlen(value)); break;
    case T_SKEL_DIR:
      c->skel_dir = (char *)malloc(sizeof(char *) * strlen(value));
      memset(c->skel_dir, 0, strlen(value)); memcpy(c->skel_dir, value, strlen(value)); break;
    default:
      fprintf(stderr, "Unknown attribute: %s = %s on config\n", attribute_type_to_string(t), value);
      return -1;
  }
  return 0;
}

// Free a config object
void free_config(honeycomb_config *c) {
  assert( c != NULL );
  size_t i;
  for (i = 0; i < c->num_phases; i++) {
    if (c->phases[i]) {
      free_phase(c->phases[i]);
    }
  }
  if (c->app_type) free(c->app_type);
  if (c->root_dir) free(c->root_dir);
  if (c->env) free(c->env);
  if (c->executables) free(c->executables);
  if (c->directories) free(c->directories);
  if (c->stdout) free(c->stdout);
  if (c->stdin) free(c->stdin);
  if (c->user) free(c->user);
  if (c->group) free(c->group);
  if (c->image) free(c->image);
  if (c->skel_dir) free(c->skel_dir);
  if (c->storage_dir) free(c->storage_dir);
}

// Free a phase struct
void free_phase(phase *p) {
  if (!p) return;
  if (p->before) free(p->before);
  if (p->command) free(p->command);
  if (p->after) free(p->after);
}