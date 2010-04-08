#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

/* Types */
typedef struct _process_t_ {
  char**  env;
  int     env_c;
  int     env_capacity;
  char*   command;
  char*   before;
  char*   after;
  char*   cd;
} process_t;

/* Helpers */
int pm_new_process(process_t **ptr);

/* External exports */
int pm_check_pid_status(pid_t pid);
int pm_add_env(process_t **ptr, char *str);
int pm_process_valid(process_t **ptr);

int pm_free_process(process_t *p);
int pm_malloc_and_set_attribute(char **ptr, char *value);

#endif