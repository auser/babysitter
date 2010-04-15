#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>             // For timeval struct
#include <sys/wait.h>             // For waitpid
#include <setjmp.h>

#include "process_manager.h"
#include "pm_helpers.h"
#include "ei_decode.h"
#include "print_helpers.h"

#define BUF_SIZE 128

/**
* Globals ewww
**/
extern process_struct*  running_children;
extern process_struct*  exited_children;
struct sigaction        mainact;
extern int              terminated;         // indicates that we got a SIGINT / SIGTERM event
int                     run_as_user;
pid_t                   process_pid;
extern int              dbg;
int                     read_handle = 0;
int                     write_handle = 1;

int setup()
{
  run_as_user = getuid();
  process_pid = getpid();
  pm_setup(read_handle, write_handle);
    
  return 0;
}

const char* cli_argument_required(int argc, char **argv[], const char* msg) {
  if (!(*argv)[2]) {
    fprintf(stderr, "A second argument is required for argument %s\n", msg);
    return NULL;
  }
  char *ret = (*argv)[2];
  (argc)--; (*argv)++;
  return ret;
}

int parse_the_command_line(int argc, const char** argv)
{
  const char* arg = NULL;
  while (argc > 1) {
    if (!strncmp(argv[1], "--debug", 7) || !strncmp(argv[1], "-d", 2)) {
      arg = argv[2];
      argc--; argv++; char * pEnd;
      dbg = strtol(arg, &pEnd, 10);
    } else if (!strncmp(argv[1], "--read_handle", 13) || !strncmp(argv[1], "-r", 2)) {
      arg = argv[2]; argc--; argv++; char * pEnd;
      read_handle = strtol(arg, &pEnd, 10);
    } else if (!strncmp(argv[1], "--write_handle", 14) || !strncmp(argv[1], "-w", 2)) {
      arg = argv[2]; argc--; argv++; char * pEnd;
      write_handle = strtol(arg, &pEnd, 10);
    } else if (!strncmp(argv[1], "--non-standard", 14) || !strncmp(argv[1], "-n", 2)) {
      read_handle = 3; write_handle = 4;
    } else if (!strncmp(argv[1], "--non-blocking", 14) || !strncmp(argv[1], "-b", 2)) {
      fcntl(read_handle,  F_SETFL, fcntl(read_handle,  F_GETFL) | O_NONBLOCK);
      fcntl(write_handle, F_SETFL, fcntl(write_handle, F_GETFL) | O_NONBLOCK);
    }
    argc--; argv++;
  }
  return 0;
}

void erl_d_gotsignal(int signal)
{
  debug(dbg, 1, "erlang daemon got a signal: %d\n", signal);
  if (signal == SIGTERM || signal == SIGINT || signal == SIGPIPE)
    terminated = 1;
}

void setup_erl_daemon_signal_handlers()
{
  mainact.sa_handler = erl_d_gotsignal;
  sigemptyset(&mainact.sa_mask);
  sigaddset(&mainact.sa_mask, SIGCHLD);
  mainact.sa_flags = 0;
  sigaction(SIGINT,  &mainact, NULL);
  sigaction(SIGTERM, &mainact, NULL);
  sigaction(SIGHUP,  &mainact, NULL);
  sigaction(SIGPIPE, &mainact, NULL);
}

int terminate_all()
{
  return 0;
}

// Fancy printing...
void print_ellipses(int count)
{
  if (count < 1) return;
  usleep(1000000);
  printf(".");
  fflush(stdout);
  print_ellipses(count - 1);
}

int decode_and_run_erlang(unsigned char *buf, int len)
{
  process_t *process;
  ei_decode_command_call_into_process((char *)buf, &process);
  
  // Do something here
  pid_t pid = pm_run_process(process);
  
  process_struct *ps = (process_struct *) calloc(1, sizeof(process_struct));
  ps->pid = pid;
  ps->transId = process->transId;
  HASH_ADD_INT(running_children, pid, ps);
  ei_pid_ok(write_handle, process->transId, pid);
  
  pm_free_process(process);
  return 0;
}

void child_changed_status(pid_t pid, int status)
{
  // A child was affected (in the following ways)
  printf("child_changed_status callback called\n");
}

int main (int argc, char const *argv[])
{
  // Consider making this multi-plexing
  fd_set rfds, wfds;      // temp file descriptor list for select()
  int rnum = 0, wnum = 0;
      
  if (parse_the_command_line(argc, argv)) return 0;
  
  setup_erl_daemon_signal_handlers();
  if (setup()) return -1;
  
  // Create the timeout struct
  struct timeval m_tv;
  m_tv.tv_usec = 0; 
  m_tv.tv_sec = 5;
    
  /* Do stuff */
  while (!terminated) {    
    debug(dbg, 4, "preparing next loop...\n");
    if (pm_next_loop(child_changed_status) < 0) break;
    
    // Erlang fun... pull the next command from the read_fds parameter on the erlang fd
    pm_set_can_not_jump();
    
    // clean socket lists
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		wnum = -1;
    
    FD_SET(read_handle, &rfds);
    // FD_SET(write_handle, &wfds);
		rnum = read_handle;
    
    // Block until something happens with select
    int num_ready_socks = select(
			(wnum > rnum) ? wnum+1 : rnum+1,
			(-1 != rnum)  ? &rfds : NULL,
			(-1 != wnum)  ? &wfds : NULL, // never will write... yet?
			(fd_set *) 0,
			&m_tv
		);
    debug(dbg, 4, "number of ready sockets from select: %d\n", num_ready_socks);
    
    int interrupted = (num_ready_socks < 0 && errno == EINTR);
    pm_set_can_jump();
    
    if (interrupted || num_ready_socks == 0) {
      if (pm_check_children(child_changed_status, terminated) < 0) continue;
    } else if (num_ready_socks < 0) {
      perror("select"); 
      exit(9);
    } else if ( FD_ISSET(read_handle, &rfds) ) {
      // Read from read_handle a command sent by Erlang
      unsigned char* buf;
      int len = 0;
      
      if ((len = ei_read(read_handle, &buf)) < 0) {
        terminated = len;
        break;
      }
      
      if (decode_and_run_erlang(buf, len)) {
        // Something is afoot (failed)
      } else {
        // Everything went well
      }
    }
  }
  terminate_all();
  return 0;
}