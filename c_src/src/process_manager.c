#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "process_manager.h"

int pm_check_pid_status(pid_t pid)
{
  if (pid < 1) return -1; // Illegal
  return kill(pid, 0);
}

int pm_new_process(process_t **ptr)
{
  process_t *p = (process_t *) calloc(1, sizeof(process_t));
  if (!p) {
    perror("Could not allocate enough memory to make a new process.\n");
    return -1;
  }
  
  p->env_c = 0;
  p->env = NULL;
  
  p->command = NULL;
  
  *ptr = p;
  return 0;
}

// Privates
int malloc_and_set_attribute(char **ptr, char *value)
{
  char *obj = (char *) calloc (sizeof(char), strlen(value) + 1);
  
  if (!obj) {
    perror("malloc_and_set_attribute");
    return -1;
  }
  printf("setting obj (%p) to %s\n", obj, value);
  strncpy(obj, value, strlen(value));
  obj[strlen(value)] = (char)'\0';
  *ptr = obj;
  return 0;
}
