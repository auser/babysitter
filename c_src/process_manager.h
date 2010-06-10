#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <signal.h>
#include <sys/time.h>             // For timeval struct
#include <sys/wait.h>             // For waitpid
#include <time.h>                 // For time function

#include <setjmp.h>
#include "uthash.h"
#include "pm_helpers.h"

#include "print_helpers.h"

enum ProcessReturnState {PRS_BEFORE,PRS_AFTER,PRS_COMMAND,PRS_OKAY};
typedef struct _process_return_t_ {
  int exit_status;  // The exit status at the stage
  pid_t pid;        // The pid of the main process
  char* stdout;     // The stdout from the process
  char* stderr;     // The stderr from the process
  enum ProcessReturnState stage;        // At what stage the process exited
} process_return_t;

/* Types */
typedef struct _process_t_ {
  char**  env;
  int     env_c;
  int     env_capacity;
  char*   command;
  char*   before;
  char*   after;
  char*   cd;
  char*   stdout;
  char*   stderr;
  int     nice;
  pid_t   pid;            // Used only when kill is the action
  int     transId;        // Communication id
} process_t;

typedef struct _process_struct_ {
    pid_t pid;                  // key
    pid_t kill_pid;             // Kill pid
    time_t deadline;            // Deadline to kill the pid
    int status;                 // Status of the pid
    int transId;                // id of the transmission
    UT_hash_handle hh;          // makes this structure hashable
} process_struct;

/* Helpers */
#define DUP2CLOSE(oldfd, newfd)	( dup2(oldfd, newfd), close(oldfd) )
int remap_pipe_stdin_stdout(int rpipe, int wpipe);
int pm_new_process(process_t **ptr);
process_return_t* pm_new_process_return();

/* External exports */
int pm_check_pid_status(pid_t pid);
int pm_add_env(process_t **ptr, char *str);
int pm_process_valid(process_t **ptr);
int pm_free_process(process_t *p);
int pm_free_process_return(process_return_t *p);

int pm_malloc_and_set_attribute(char **ptr, char *value);
void pm_set_can_jump();
void pm_set_can_not_jump();

/* extra helpers */
int pm_setup(int read_handle, int write_handle);

/* Mainly private exports */
process_return_t* pm_run_and_spawn_process(process_t *process);
process_return_t* pm_run_process(process_t *process);
int pm_kill_process(process_t *process);

pid_t pm_execute(int should_wait, const char* command, const char *cd, int nice, const char** env, int* stdout);
int pm_check_children(void (*child_changed_status)(process_struct *ps), int isTerminated);
int pm_check_pending_processes();
int pm_next_loop(void (*child_changed_status)(process_struct *ps));

#endif
