#include "process_manager.h"

process_struct*     running_children;
process_struct*     exited_children;
sigjmp_buf          saved_jump_buf;
int                 pm_can_jump = 0;
int                 terminated = 0;
int                 signaled   = 0;     // indicates that SIGCHLD was signaled
int                 dbg = 0;
static int safe_chdir(const char *);

char* read_from_pipe(int fd) {
  if (fd < 0) return NULL;
  char buf[MAX_BUFFER_SZ];
  int numread = read(fd, buf, sizeof(buf));
  buf[numread] = '\0';
  char *out = (char*) calloc(1, sizeof(char)*strlen(buf));
  strncpy(out, buf, strlen(buf));
  return out;
}

int pm_check_pid_status(pid_t pid)
{
  if (pid < 1) return -1; // Illegal
  int err = kill(pid, 0);
  // switch (err) {
  //   case 0:
  //   case EINVAL:
  //   case ESRCH:
  //   case EPERM:
  //   default:
  // }
  return err;
}

int pm_add_env(process_t **ptr, char* value)
{
  process_t *p = *ptr;
  int old_size = p->env_c;
  // Expand space, if necessary
  if (p->env_c == p->env_capacity) {
    if (p->env_capacity == 0) p->env_capacity = 1 * sizeof(char*);
    int new_size = (p->env_capacity *= 2) * sizeof(char*);
    void *new_env = (char**)realloc(p->env, new_size);
    if (new_env != NULL) {
      p->env = new_env;
      // Clear out the new mem
      p->env[old_size+1] = NULL;
    } else {
      return -1; // Something is SERIOUSLY wrong
    }
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
  p->nice = 0;
  
  p->command = NULL;
  p->before = NULL;
  p->after = NULL;
  
  *ptr = p;
  
  return 0;
}

process_return_t* pm_new_process_return()
{
  process_return_t *p = (process_return_t *)calloc(1, sizeof(process_return_t));
  if (!p) {
    perror("Could not allocated enough memory to make a new process return type\n");
    return NULL;
  }
  p->pid = 0;
  p->exit_status = 0;
  p->stage = PRS_BEFORE;
  
  p->stdout = NULL;
  p->stderr = NULL;
  
  return p;
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

int pm_free_process_return(process_return_t *p)
{
  if (p->stdout) free(p->stdout);
  if (p->stderr) free(p->stderr);
  
  free(p);
  return 0;
}

int pm_free_process(process_t *p)
{
  if (p->command) free(p->command);
  if (p->before) free(p->before);
  if (p->after) free(p->after);
  if (p->cd) free(p->cd);
  if (p->stdout) free(p->stdout);
  if (p->stderr) free(p->stderr);
  
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
      syslog(LOG_WARNING, "Received SIGHUP signal.");
      break;
    case SIGKILL:
    case SIGTERM:
      syslog(LOG_WARNING, "Received SIGTERM signal.");
      exit(0);
    break;
    case SIGINT:
      syslog(LOG_WARNING, "Received SIGINT signal.");
    break;
    default:
      syslog(LOG_WARNING, "Unhandled signal (%d) %s", strsignal(signal));
    break;
  }
}

void pm_gotsigchild(int signal, siginfo_t* si, void* context)
{
  // If someone used kill() to send SIGCHLD ignore the event
  if (signal != SIGCHLD) return;
  signaled = 1;
  // process_child_signal(si->si_pid);
}
void pm_setup_fork()
{
  // int i = getdtablesize();
  // for (i=getdtablesize();i>=0;--i) close(i);
}
/**
* Setup signal handlers for the process
**/
void pm_setup_child()
{
  // Set signal handlers first
  struct sigaction sact, sterm;
  sterm.sa_handler = pm_gotsignal;
  sigemptyset(&sterm.sa_mask);
  sigaddset(&sterm.sa_mask, SIGCHLD);
  sterm.sa_flags = 0;
  sigaction(SIGINT,  &sterm, NULL);
  sigaction(SIGKILL,  &sterm, NULL);
  sigaction(SIGTERM, &sterm, NULL);
  sigaction(SIGHUP,  &sterm, NULL);
  sigaction(SIGPIPE, &sterm, NULL);
  
  sact.sa_handler = NULL;
  sact.sa_sigaction = pm_gotsigchild;
  sigemptyset(&sact.sa_mask);
  sact.sa_flags = SA_SIGINFO | SA_RESTART | SA_NOCLDSTOP | SA_NODEFER;
  sigaction(SIGCHLD, &sact, NULL);
	  
  // Change the file mode mask
  umask(027);
}

int pm_setup(int read_handle, int write_handle)
{
#ifdef _WIN32
  /* Attention Windows programmers: you need to explicitly set
   * mode of stdin/stdout to binary or else the port program won't work
   */
  setmode(read_handle, O_BINARY);
  setmode(write_handle, O_BINARY);
#endif
  return 0;
}

int expand_command(const char* command, int* argc, char ***argv, int *using_a_script, const char** env)
{
  char **command_argv = *argv;
  int command_argc = *argc;
  int running_script = *using_a_script;
  const char *full_filepath;
  
  if (!strncmp(command, "#!", 2)) {
    // We are running a shell script command
    char *filename = strdup("/tmp/babysitter.XXXXXXX");
    int size, fd = -1;
    FILE *sfp;
    // Note for the future cleanup, that we'll be running a script to cleanup
    running_script = 1;
    
    // Make a tempfile in the filename format
    // printf("writing a tempfile: %s\n", filename);
    if ((fd = mkstemp(filename)) == -1 || (sfp = fdopen(fd, "w+")) == NULL) {
      if (fd != -1) {
        unlink(filename);
        close(fd);
      }
      fprintf(stderr, "Could not open tempfile: %s\n", filename);
      return -1;
    }
    
    size = strlen(command);
    // Print the command into the file
    if (fwrite(command, size, 1, sfp) == -1) {
      fprintf(stderr, "Could not write command to tempfile: %s\n", filename);
      return -1;
    }
    fclose(sfp);
    // Close the file descriptor
    close(fd);

    // Modify the command to match call the filename
    // should we chown?
    /*
		if (chown(filename, m_user, m_group) != 0) {
#ifdef DEBUG
     fprintf(stderr, "Could not change owner of '%s' to %d\n", m_script_file.c_str(), m_user);
#endif
		}
		*/

    // Make it executable
    if (chmod(filename, 040700) != 0) {
      fprintf(stderr, "Could not change permissions to '%s' %o\n", filename, 040700);
    }

    // Run in a new process
    command_argv = (char **) malloc(1 * sizeof(char *));
    command_argv[0] = strdup(filename);
    command_argv[1] = NULL;
    command_argc = 1;
    free(filename);
  } else {
    int prefix;
    char *cp, *cmdname, *expanded_command;
    char *path = NULL;

    // get bare command for path lookup
    for (cp = (char *) command; *cp && !isspace(*cp); cp++) ;
    prefix = cp - command;
    cmdname = calloc(prefix, sizeof(char));
    strncpy(cmdname, command, prefix);

    // Extract the PATH, if given
    if (env) {
      char **tmp_env = (char**)env;
      while(*tmp_env != NULL) {
        if (!strncmp(*tmp_env, "PATH=", 5)) {
          // Get past the first PATH=
          // GROSS - pointer arithmatic
          path = (*tmp_env + sizeof(char)*5);
          break;
        }
        tmp_env++;
      }
    }
    
    // expand command name to full path
    full_filepath = find_binary(cmdname, path);
    
    // build invocable command with args
    expanded_command = calloc(strlen(full_filepath) + strlen(command + prefix) + 1, sizeof(char));
    strcat(expanded_command, full_filepath); 
    strcat(expanded_command, command + prefix);
    
    command_argv = (char **) malloc(4 * sizeof(char *));
    command_argv[0] = strdup(getenv("SHELL"));
    command_argv[1] = "-c";
    command_argv[2] = expanded_command;
    command_argv[3] = NULL;
    command_argc = 3;
  }
  *argc = command_argc;
  *argv = command_argv;
  *using_a_script = running_script;
  
  return 0;
}

/**
* pm_execute
* @params
*   int should_wait - Indicates if this execute should be inline or asynchronous
*   const char* command - The command to run
*   const char* cd - Run in this directory unless it's a NULL pointer
*   int nice - Special nice level
*   const char** env - Environment variables to run in the shell
* @output
    pid_t pid - output pid of the new process
**/
pid_t pm_execute(
  int should_wait, const char* command, const char *cd, int nice, const char** env, int *child_stdin, const char* p_stdout, const char *p_stderr
) {
  // Setup execution
  char **command_argv = {0};
  int command_argc = 0;
  int running_script = 0;
  int countdown = 200;
  
  // If there is nothing here, don't run anything :)
  if (strlen(command) == 0) return -1;
  
  char* chomped_string = str_chomp(command);
  char* safe_chomped_string = str_safe_quote(chomped_string);
  if (expand_command((const char*)safe_chomped_string, &command_argc, &command_argv, &running_script, env)) ;
  command_argv[command_argc] = 0;
      
  // Now actually RUN it!
  pid_t pid;
  
#if USE_PIPES

  int child_fd[2];
  if ( pipe(child_fd) < 0 ) {// Create a pipe to the child (do we need this? I doubt it)
    perror("pipe failed");
  }
  // Setup the stdin so the parent can communicate!
  (*child_stdin) = child_fd[0];
#endif

    // Let's name it so we can get to it later
  int child_dev_null;
  
#if DEBUG
  if ((child_dev_null = open("debug.log", O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0) {
    syslog(LOG_ERR, "babysitter (fatal): Could not open debug.log: errno: %d\n", errno);
  };
#else
  if ((child_dev_null = open("/dev/null", O_RDWR)) < 0) {
    syslog(LOG_ERR, "babysitter (fatal): Could not open /dev/null: errno: %d\n", errno);
  };
#endif
    
  // Setup fork
  pm_setup_fork();
  
  if (should_wait)
    pid = vfork();
  else
    pid = fork();
    
  switch (pid) {
  case -1: 
    return -1;
  case 0: {
    // Child process
    pm_setup_child();
    
    if (cd != NULL && cd[0] != '\0')
      safe_chdir(cd);
    else
      safe_chdir("/tmp");
        
    int child_stdout, child_stderr;
    // Set everything to dev/null first
    child_stdout = child_stderr = child_dev_null;
    
    // If we've passed in a stdout filename, then open it
    if(p_stdout)
      if ((child_stdout = open(p_stdout, O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0) {
        perror("child stdout");
        child_stdout = child_dev_null;
      }
    // If we've been passed a stderr filename, then open that
    if(p_stderr)
      if ((child_stderr = open(p_stderr, O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0) {
        perror("child stderr");
        child_stderr = child_dev_null;
      }
    
    // Parent doesn't write anything to the child, so just close this right away
    // REDIRECT TO DEV/NULL
    // Replace the stdout/stderr with the child_write fd
#if USE_PIPES
    if (dup2(child_fd[0], STDIN_FILENO) < 0)       FATAL_ERROR("could not dup STDIN_FILENO", -1);
    close(child_fd[1]); // We are using a different stdout
#endif

#if DEBUG
#else
    // Setup the different stdout/stderr
    if (dup2(child_stdout, STDOUT_FILENO) < 0)  FATAL_ERROR("could not dup STDOUT_FILENO", -1);
    if (dup2(child_stderr, STDERR_FILENO) < 0)  FATAL_ERROR("could not dup STDERR_FILENO", -1);

    if (child_stdout != child_dev_null) close(child_stdout);
    if (child_stderr != child_dev_null) close(child_stderr);
#endif

    if (execve((const char*)command_argv[0], command_argv, (char* const*) env) < 0) {
      perror("execve");
      exit(-1);
    }
  }
  default:
    // In parent process
    // set the stdout back
    close(child_dev_null); // Child write never gets used outside the child
  
#if USE_PIPES
    if (dup2(child_fd[1], STDOUT_FILENO) < 0)
      perror("dup2");
    
    close(child_fd[0]);
#endif
    
    if (nice != INT_MAX && setpriority(PRIO_PROCESS, pid, nice) < 0) 
      ;
    if (running_script) {
      while (countdown > 0) {
        if (kill(pid, 0) != 0) break;
        usleep(100);
        countdown--;
      }
      struct stat buffer;
      if (stat(command_argv[0], &buffer) != 0) {
        printf("file doesn't exist when it should because: %s\n", strerror(errno));
      }
      if( unlink( command_argv[0] ) != 0 ) perror( "Error deleting file" );
    }
    // These are free'd later, anyway
    // if (chomped_string) free(chomped_string); 
    // if (safe_chomped_string) free(safe_chomped_string);
    return pid;
  }
}

int wait_for_pid(pid_t pid, int opts)
{
  if (pid < 0) return pid;
  int stat = 0;
  int childExitStatus;
  stat = waitpid(pid, &childExitStatus, opts);
  if (stat == -1) {
    perror("wait_for_pid");
    return pid;
  } else {
    if (WIFEXITED(childExitStatus))
      return WEXITSTATUS(childExitStatus);
    else
      return 0; // We have not exited
  }
}

typedef enum HookT {BEFORE_HOOK, AFTER_HOOK} hook_t;
int run_hook(hook_t t, process_t *process, process_return_t *ret)
{
  int child_stdin;
  if (t == BEFORE_HOOK) {
    ret->stage = PRS_BEFORE;
    ret->pid = pm_execute(1,  (const char*)process->before, 
                              (const char*)process->cd, 
                              (int)process->nice, 
                              (const char**)process->env, 
                              &child_stdin,
                              (const char*)process->stdout, (const char*)process->stderr);
  } else if (t == AFTER_HOOK) {
    ret->stage = PRS_AFTER;
    ret->pid = pm_execute(1,  (const char*)process->after, 
                              (const char*)process->cd, 
                              (int)process->nice, 
                              (const char**)process->env, 
                              &child_stdin,
                              (const char*)process->stdout, (const char*)process->stderr);
  }
  // Check to make sure the hook executed properly
  ret->exit_status = wait_for_pid(ret->pid, 0);
  if (ret->exit_status != 0) {
    if (errno) {
      if (process->stdout) ret->stdout = read_from_file((const char*)process->stdout);
      if (process->stderr) {
        ret->stderr = read_from_file((const char*)process->stderr);
      } else {
        ret->stderr = (char*)calloc(1, sizeof(char)*strlen(strerror(errno)));
        strncpy(ret->stderr, strerror(errno), strlen(strerror(errno)));
      }
    }
    
    return -1;
  }
  return 0;
}

process_return_t* pm_run_and_spawn_process(process_t *process)
{
  process_return_t* ret = pm_new_process_return();
  // Setup the stdin
  int child_stdin; 
  if (ret == NULL) return NULL;
  // make a pidfile
  char pidfile[43];
  if (tmpnam(pidfile) == NULL) {perror("pidfile");}
  
  // char *pidfile = tmpname(strdup("/tmp/babysitter.pidfile.XXXXXXXXX"));
  char *pidfile_env = (char*) calloc(1, sizeof(char)*43);
  strncpy(pidfile_env, "PID_FILE=", 10);
  strncat(pidfile_env, pidfile, 33);
    // Add the pidfile to the run
  pm_add_env(&process, pidfile_env);
  // new_process_return
  if (process->env) process->env[process->env_c] = NULL;
  
  // Run afterhook
  if (process->before) {
    if (run_hook(BEFORE_HOOK, process, ret)) return ret;
  }
  
  ret->stage = PRS_COMMAND;
  ret->pid = pm_execute(0,  (const char*)process->command, 
                            (const char*)process->cd, 
                            (int)process->nice, 
                            (const char**)process->env, 
                            &child_stdin,
                            (const char*)process->stdout, (const char*)process->stderr);
                          
  pid_t pid_from_pidfile;
  // If the program wrote to the pidfile, we'll want to use that instead, so check
  if (( pid_from_pidfile = (pid_t)get_pid_from_file_or_retry(pidfile, 50)) > 0) {
    ret->pid = pid_from_pidfile;
  };
  
  unlink(pidfile); // Clean up after ourselves
  if (errno) {
    if (process->stdout) ret->stdout = read_from_file((const char*)process->stdout);
    
    if (process->stderr) {
      ret->stderr = read_from_file((const char*)process->stderr);
    } else {
      ret->stderr = (char*)calloc(1, sizeof(char)*strlen(strerror(errno)));
      strncpy(ret->stderr, strerror(errno), strlen(strerror(errno)));
    }
  }
  
  if (ret->pid < 0) {
    return ret;
  }
  
  ret->exit_status = wait_for_pid(ret->pid, WNOHANG);
  if (ret->exit_status) {
    // printf("command: %s pid: %d exit_status: %d - %s / %s\n", process->command, ret->pid, ret->exit_status, ret->stdout, ret->stderr);
    return ret;
  }
  
  // Run afterhook
  if (process->after) {
    if (run_hook(AFTER_HOOK, process, ret)) return ret;
  }

  // Yay, we finished properly
  ret->stage = PRS_OKAY;
  process_struct *ps = (process_struct *) calloc(1, sizeof(process_struct));
  ps->pid = ret->pid;
#if USE_PIPES
  ps->stdin = child_stdin;
#endif
  ps->transId = process->transId;
  HASH_ADD_INT(running_children, pid, ps);
  
  return ret;
}

/**
* Run the process
**/
process_return_t* pm_run_process(process_t *process)
{
  process_return_t* ret = pm_new_process_return();
  // Setup the stdin
  int child_stdin;
  if (ret == NULL) return NULL;
  
  if (process->env) process->env[process->env_c] = NULL;
  
  // Run afterhook
  if (process->before) {
    if (run_hook(BEFORE_HOOK, process, ret)) return ret;
  }
    
  ret->stage = PRS_COMMAND;
  pid_t pid = pm_execute(1,   (const char*)process->command, 
                              (const char*)process->cd, 
                              (int)process->nice, 
                              (const char**)process->env, 
                              &child_stdin,
                              (const char*)process->stdout, (const char*)process->stderr);
  
  ret->pid = pid;
  ret->exit_status = wait_for_pid(pid, 0);
  
  if (ret->exit_status) {
    if (errno) {
      if (process->stdout) ret->stdout = read_from_file((const char*)process->stdout);

      if (process->stderr) ret->stderr = read_from_file((const char*)process->stderr);
      else {
        ret->stderr = (char*)calloc(1, sizeof(char)*strlen(strerror(errno)));
        strncpy(ret->stderr, strerror(errno), strlen(strerror(errno)));
      }
    }
    return ret;
  }
  
  // Run afterhook
  if (process->after) {
    if (run_hook(AFTER_HOOK, process, ret)) return ret;
  }
  
  // Yay, we finished properly
  ret->stage = PRS_OKAY;
  return ret;
}

int pm_kill_process(process_t *process)
{
  pid_t pid = process->pid;
  
  // Don't want to send a negative pid
  if (pid < 1) return -1;
    
  // process_struct *ps;
  
  // for( ps = running_children; ps != NULL; ps = ps->hh.next ) {
  //   if (pid == ps->pid)
  //     break;
  // }
  // HASH_FIND_INT(running_children, (int)process->pid, ps);
  
#if USE_PIPES
#endif
    // fprintf(stderr, "pm_kill_process: %d\n", (int)pid);
  // if (ps) {
    int childExitStatus = -1;
    // Kill here
    kill(pid, SIGKILL);
    waitpid( pid, &childExitStatus, 0 );
    return 0;
  // } else {
  //   return -1;
  // }
}

int pm_check_children(void (*child_changed_status)(process_struct *ps), int isTerminated)
{
  process_struct *ps;
  int p_status = 0;
  int status;
  
  // Run through each of the running children and poke at them to see
  // if they are running or not
  for( ps = running_children; ps != NULL; ps = ps->hh.next ) {
    // Prevent zombies...
    while ((p_status = waitpid(ps->pid, &status, WNOHANG)) < 0 && errno == EINTR);
    
    if ((p_status = pm_check_pid_status(ps->pid)) > 0) {
      time_t now;
      now = time(NULL); // Current time
      // Something is wrong with the process, whatever could it be? Did we try to kill it?
      if (ps->kill_pid > 0 && difftime(ps->deadline, now) > 0) {
        // We've definitely sent this pid a shutdown and the deadline has clearly passed, let's force kill it
        kill(ps->pid, SIGTERM);
        // Kill the killing process too
        if ((p_status = kill(ps->kill_pid, 0)) == 0) kill(ps->kill_pid, SIGKILL);
        // Set the deadline for the pid
        ps->deadline += 5;
      }
      // Now wait for it, only if it's going to die.
      if (p_status > 0) {
        HASH_ADD(hh, exited_children, pid, sizeof(ps), ps);
        HASH_DEL(running_children, ps);
        continue;
      }
    } else if (p_status < 0 && errno == ESRCH) {
      // Now if the pid has most definitely disappeared, then we can 
      // send the status change and remove the pid from tracking
      HASH_DEL(running_children, ps);
      ps->status = ESRCH;
      child_changed_status(ps);
    }
  }

  return 0;
}

int pm_next_loop(void (*child_changed_status)(process_struct *ps))
{
  sigsetjmp(saved_jump_buf, 1); pm_can_jump = 0;

  while (!terminated && (HASH_COUNT(exited_children) > 0 || signaled)) pm_check_children(child_changed_status, terminated);
  pm_check_pending_processes(); // Check for pending signals arrived while we were in the signal handler
  errno = 0;

  pm_can_jump = 1;
  if (terminated) return -1;
  else return 0;
}

void pm_set_can_jump() {pm_can_jump = 1;}
void pm_set_can_not_jump() {pm_can_jump = 0;}

int setup_pm_pending_alarm()
{
  struct itimerval tval;
  struct timeval interval = {0, 20000};
  
  tval.it_interval = interval;
  tval.it_value = interval;
  setitimer(ITIMER_REAL, &tval, NULL);
  
  struct sigaction spending;
  spending.sa_handler = pm_gotsignal;
  sigemptyset(&spending.sa_mask);
  spending.sa_flags = 0;
  sigaction(SIGALRM, &spending, NULL);
  
  return 0;
}


int pm_check_pending_processes()
{
  int sig = 0;
  sigset_t sigset;
  setup_pm_pending_alarm();
  if ((sigemptyset(&sigset) == -1)
      || (sigaddset(&sigset, SIGALRM) == -1)
      || (sigaddset(&sigset, SIGINT) == -1)
      || (sigaddset(&sigset, SIGTERM) == -1)
      || (sigprocmask( SIG_BLOCK, &sigset, NULL) == -1)
     )
    perror("Failed to block signals before sigwait\n");
  
  if (sigpending(&sigset) == 0) {
    while (errno == EINTR)
      if (sigwait(&sigset, &sig) == -1) {
        perror("sigwait");
        return -1;
      }
    pm_gotsignal(sig);
  }
  return 0;
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

static int safe_chdir(const char *pathname)
{
  int res;
  if ((res = chdir(pathname)) < 0) {
    perror("chdir failed");
  }

  return res;
}
