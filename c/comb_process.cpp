/**** Includes *****/
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>

#include <sys/types.h>  // for kill() command
#include <signal.h>     // for kill() command
#include <sys/wait.h>   // for wait()
#include <stdlib.h>     // for setenv()
#include <time.h>       // for strftime()
#include <libgen.h>     // for basename()

#include <errno.h>

#include "comb_process.h"

extern int dbg;
int gbl_child_pid;
char *gbl_pidfile;
callback_t gbl_callback;

// Signal handlers
// We've received a signal to process
void gotsignal(int sig)
{
  debug(dbg, 1, "got signal: %d\n", sig);
  switch (sig) {
    case SIGTERM:
    case SIGINT:
    if (gbl_callback != NULL) gbl_callback((int)gbl_child_pid);
    kill(gbl_child_pid, sig);
    unlink(gbl_pidfile); // Cleanup the pidfile
    _exit(0);
    break;
    case SIGHUP:
    _exit(0);
    default:
    break;
  }
}

/**
 * We received a signal for the child pid process
 * Wait for the pid to exit if it hasn't only if the and it's an interrupt process
 * make sure the process get the pid and signal to it
 **/
int process_child_signal(pid_t pid)
{
  return 0;
}
/**
 * Got a signal from a child
 **/
void gotsigchild(int signal, siginfo_t* si, void* context)
{
  // If someone used kill() to send SIGCHLD ignore the event
  if (si->si_code == SI_USER || signal != SIGCHLD) return;
  
  debug(dbg, 1, "CombProcess %d exited (sig=%d)\r\n", si->si_pid, signal);
  process_child_signal(si->si_pid);
}
/***** Comb process *****/
/**
* Handle the signals to this process
*
* Note: Must use a "trampoline" function to send the 
* process to
**/
int CombProcess::setup_signal_handlers()
{
  // Ignore signals
  m_signore.sa_handler = SIG_IGN;
  sigemptyset(&m_signore.sa_mask);
  m_signore.sa_flags = 0;
  sigaction(SIGQUIT,  &m_signore, NULL); // ignore Quit
  sigaction(SIGABRT,  &m_signore, NULL); // ignore ABRT
  sigaction(SIGTSTP,  &m_signore, NULL);
  
  // Termination signals to handle with gotsignal
  m_sterm.sa_handler = gotsignal;
  sigemptyset(&m_sterm.sa_mask);
  sigaddset(&m_sterm.sa_mask, SIGCHLD);
  m_sterm.sa_flags = 0;
  
  if (sigaction(SIGINT,  &m_sterm, NULL) < 0) fprintf(stderr, "Error setting INT action\n"); // interrupts
  if (sigaction(SIGTERM, &m_sterm, NULL) < 0) fprintf(stderr, "Error setting TERM handler\n"); // ignore TERM
  if (sigaction(SIGHUP,  &m_sterm, NULL) < 0) fprintf(stderr, "Error setting HUP handler\n"); // ignore hangups

  // SIGCHLD handler
  m_sact.sa_handler = NULL;
  m_sact.sa_sigaction = &gotsigchild;
  sigemptyset(&m_sact.sa_mask);
  m_sact.sa_flags = SA_SIGINFO | SA_RESTART | SA_NOCLDSTOP | SA_NODEFER;
  sigaction(SIGCHLD, &m_sact, NULL);
  
  debug(m_dbg, 2, "Set up signal handlers\n");
  return 0;
}

/**
* Start the process
**/
pid_t CombProcess::monitored_start(int argc, char const *argv[], char **envp)
{
  set_input(argc, argv);
  set_env(envp);
  return monitored_start();
}
pid_t CombProcess::monitored_start(int argc, char const *argv[], char **envp, pid_t p_pid)
{
  set_input(argc, argv);
  set_env(envp);
  return monitored_start(p_pid);
}

pid_t CombProcess::monitored_start()
{
  pid_t pid = getpid(); // get the CombProcess ID of procautostart
  debug(m_dbg, 2, "Parent process: %d\n", (int) pid);
  return monitored_start(pid);
}

// Entry point
pid_t CombProcess::monitored_start(pid_t p_pid)
{
  struct timespec *req;
  m_parent_pid = p_pid;
  
  unsigned long tsecs = time(NULL);
  
  if (m_pidfile[0] == 0) sprintf(m_pidfile, "%s/%d%lu.pid", PID_ROOT, (unsigned int)m_parent_pid, tsecs);
  
  // Copy to the global pid file for safe-keeping
  gbl_pidfile = (char *)malloc(sizeof(char) * strlen(m_pidfile));
  memset(gbl_pidfile, 0, sizeof(char) * strlen(m_pidfile)); 
  strncpy(gbl_pidfile, m_pidfile, strlen(m_pidfile));
  
  mkdir_p(dirname(m_pidfile));
  
  m_process_pid = start_process(p_pid);
  // detach from the main process if 0 returned in child process
  if (fork()) {
    return m_process_pid;
    // exit(0); // non-zero child PID
  }
  
  if (m_process_pid < 0) {
    FATAL_ERROR(stderr, "Failed to start the process\n");
    exit(-1);
  }
  sleep(INITIAL_PROCESS_WAIT); // Give the process a some time to start
  
  while (1) {
    switch (kill(m_process_pid, 0)) {
      case 0:
      break;
      default:
      case ESRCH:
        debug(m_dbg, 1, "CombProcess %s died\n", m_name);
        if (m_callback != NULL) {
          m_callback((int)m_parent_pid);
          unlink(gbl_pidfile);
          exit(0);
        } else {
          m_process_pid = start_process(m_parent_pid);
          write_to_pidfile();
          sleep(INITIAL_PROCESS_WAIT); // Give the process a some time to start
        }
      break;
    }
    sleep(m_sec);
    usleep(m_micro);
    req = new struct timespec;
    req->tv_sec = 0; // seconds
    req->tv_nsec = m_nano; // nanoseconds
    nanosleep( (const struct timespec *)req, NULL);
  }
}

int CombProcess::start_process(pid_t parent_pid)
{
  pid_t child_pid = 0;
  switch(child_pid = safe_fork(parent_pid)) {
    case -1:
      fprintf(stderr, "Could not fork in process. Fatal error in CombProcess::start(): %s\n", ::strerror(errno));
      _exit(errno);
      break;
    case 0:
      // We are in the child process
      debug(m_dbg, 1, "Starting comb with the command: %s\n", m_argv[0]);
      
      // cd into the directory if there is one
      if (m_cd[0] != (char)'\0') {
        if (chdir(m_cd)) {
          perror("chdir");
        }
      }
      
      execve(m_argv[0], (char* const*)m_argv, (char* const*)m_cenv);
      
      // If execlp returns than there is some serious error !! And
      // executes the following lines below...
      fprintf(stderr, "Error: Unable to start child process\n");
      child_pid = -2;
      exit(127);
      break;
    default:
      setup_signal_handlers();
      if (child_pid < 0) fprintf(stderr, "\nFatal Error: Problem while starting child process\n");
      char buf[LIL_BUF];
      FILE *pFile;
      
      if ((pFile = fopen(m_pidfile, "r")) != NULL) {
        memset(buf, 0, LIL_BUF);
        fgets(buf, LIL_BUF, pFile);
        child_pid = atoi(buf);
      }
      fclose(pFile);
      break;
  }
  gbl_child_pid = child_pid;
  gbl_callback = m_callback;
  debug(m_dbg, 1, "Child pid %d\n", (int)child_pid);
  return child_pid;
}

/***** Utils *****/
/**
* This safe_fork forks processes and doesn't leave a zombie process
**/
pid_t CombProcess::safe_fork(pid_t parent_pid)
{
  pid_t caller_pid = -1;
  pid_t child_pid = -1;
  int   status;
  
  if (!(caller_pid = fork())) {
    switch (child_pid = fork()) {
      case 0:
        return child_pid;
      case -1:
        _exit(errno);
      default:
        m_process_pid = gbl_child_pid = child_pid;
        write_to_pidfile();
        // Leave
        _exit(0);
    }
  }
  
  if (caller_pid < 0 || waitpid(caller_pid, &status, 0) < 0) return -1;

  if (WIFEXITED(status))
    if (WEXITSTATUS(status) == 0)
      return 1;
    else
      errno = WEXITSTATUS(status);
  else
    errno = EINTR;

  return -1;
}

/**
* Write pid to pid file
**/
int CombProcess::write_to_pidfile()
{
  char buf[20];
  // Buffer
  memset(buf, 0, sizeof(buf));
  snprintf(buf, sizeof(buf) - 1, "%u\n", (unsigned int) m_process_pid);
  // Open file
  debug(m_dbg, 2, "Opening pidfile: %s and writing %s to it\n", m_pidfile, buf);
  FILE *pFile = fopen(m_pidfile, "w");
  if ((ssize_t)fwrite(buf, 1, strlen(buf), pFile) != (ssize_t)strlen(buf)) {
    fprintf(stderr, "Could not write to pid file (%s): %s\n", m_pidfile, strerror(errno));
    fclose(pFile);
    _exit(-1);
  }
  fclose(pFile);
  return 0;
}