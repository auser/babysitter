#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "process_manager.h"

int pm_check_pid_status(pid_t pid)
{
  if (pid < 1) return -1; // Illegal
  return ((kill(pid, 0) == 0) ? 0 : -1);
}

int pm_add_env(process_t **ptr, char* value)
{
  process_t *p = *ptr;
  // Expand space, if necessary
  if (p->env_c == p->env_capacity) {
    if (p->env_capacity == 0) p->env_capacity = 1;
    p->env = (char**)realloc(p->env, (p->env_capacity *= 2) * sizeof(char*));
  }
  p->env[p->env_c++] = strdup(value);
  return 0;
}

int pm_new_process(process_t **ptr)
{
  process_t *p = (process_t *) calloc(1, sizeof(process_t));
  if (!p) {
    perror("Could not allocate enough memory to make a new process.\n");
    return -1;
  }
  
  p->env_c = 0;
  p->env_capacity = 0;
  p->env = NULL;
  p->cd = NULL;
  
  p->command = NULL;
  p->before = NULL;
  p->after = NULL;
  
  *ptr = p;
  return 0;
}

/**
* Check if the process is valid
**/
int pm_process_valid(process_t **ptr)
{
  process_t* p = *ptr;
  if (p->command == NULL) return -1;
  return 0;
}

int pm_free_process(process_t *p)
{
  if (p->command) free(p->command);
  if (p->before) free(p->before);
  if (p->after) free(p->after);
  if (p->cd) free(p->cd);
  
  int i = 0;
  for (i = 0; i < p->env_c; i++) free(p->env[i]);
  if (p->env) free(p->env);
  
  free(p);
  return 0;
}

/*--- Run process ---*/
void pm_gotsignal(int signal)
{ 
  switch(signal) {
    case SIGHUP:
      break;
    case SIGTERM:
    case SIGINT:
    default:
    break;
  }
}

void pm_gotsigchild(int signal, siginfo_t* si, void* context)
{
  // If someone used kill() to send SIGCHLD ignore the event
  if (signal != SIGCHLD) return;
  
  // process_child_signal(si->si_pid);
}
/**
* Setup signal handlers for the process
**/
void setup_signal_handlers()
{
  struct sigaction                sact, sterm;
  sterm.sa_handler = pm_gotsignal;
  sigemptyset(&sterm.sa_mask);
  sigaddset(&sterm.sa_mask, SIGCHLD);
  sterm.sa_flags = 0;
  sigaction(SIGINT,  &sterm, NULL);
  sigaction(SIGTERM, &sterm, NULL);
  sigaction(SIGHUP,  &sterm, NULL);
  sigaction(SIGPIPE, &sterm, NULL);
  
  sact.sa_handler = NULL;
  sact.sa_sigaction = pm_gotsigchild;
  sigemptyset(&sact.sa_mask);
  sact.sa_flags = SA_SIGINFO | SA_RESTART | SA_NOCLDSTOP | SA_NODEFER;
  sigaction(SIGCHLD, &sact, NULL);
}

pid_t pm_run_process(process_t *process)
{
  // pid_t pid = fork();
  // switch (pid) {
  // case -1: 
  //   return -1;
  // case 0: {
  //   // We are in the child process
  //   setup_signal_handlers();
  //   const char* const argv[] = { getenv("SHELL"), "-c", command_argv, (char*)NULL };
  //   if (cd != NULL && cd[0] != '\0' && chdir(cd) < 0) {
  //     fperror("Cannot chdir to '%s'", cd);
  //     return EXIT_FAILURE;
  //   }
  //   
  //   if (execve((const char*)argv[0], (char* const*)argv, (char* const*) env) < 0) {
  //     fperror("Cannot execute %s: %s\n", argv[0], strerror(errno));
  //     exit(-1);
  //   }
  //   debug(dbg, 1, "There was an error in execve\n");
  //   exit(-1);
  // }
  // default:
  //   // In parent process
  //   if (nice != INT_MAX && setpriority(PRIO_PROCESS, pid, nice) < 0) {
  //     fperror("Cannot set priority of pid %d to %d", pid, nice);
  //   }
  //   if (pid > 0) {
  //     CmdInfo ci(command_argv, kill_cmd, pid);
  //     children[pid] = ci;
  //   }
  //   return pid;
  // }
  return -1;
}

// Privates
int pm_malloc_and_set_attribute(char **ptr, char *value)
{
  char *obj = (char *) calloc (sizeof(char), strlen(value) + 1);
  
  if (!obj) {
    perror("pm_malloc_and_set_attribute");
    return -1;
  }
  strncpy(obj, value, strlen(value));
  obj[strlen(value)] = (char)'\0';
  *ptr = obj;
  return 0;
}
