#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

/* Types */
typedef struct _process_t_ {
  char**  env;
  int     env_c;
  char*   command;
} process_t;

/* Helpers */
int pm_new_process(process_t **ptr);

/* External exports */
int pm_check_pid_status(pid_t pid);

int malloc_and_set_attribute(char **ptr, char *value);

#endif