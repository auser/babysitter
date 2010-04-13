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
char*                   buf;
int                     read_handle = 2;
int                     write_handle = 3;

int setup()
{
  run_as_user = getuid();
  process_pid = getpid();
  pm_setup(read_handle, write_handle);
  
  if ((buf = (char *) malloc( sizeof(buf) )) == NULL) 
    return -1;
  
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
      read_handle = 3;
      write_handle = read_handle + 1;
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
  // while (HASH_COUNT(running_children) > 0) {
  //   
  // }
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

int main (int argc, char const *argv[])
{
  fd_set readfds;

  if (parse_the_command_line(argc, argv)) return 0;
  
  setup_erl_daemon_signal_handlers();
  if (setup()) return -1;
  
  /* Do stuff */
  while (!terminated) {
    debug(dbg, 4, "looping... (%d)\n", (int)terminated);
    
    FD_ZERO (&readfds);
    FD_SET (read_handle, &readfds);
    
    debug(dbg, 4, "preparing next loop...\n");
    if (pm_next_loop()) break;
    
    // Erlang fun... pull the next command from the readfds parameter on the erlang fd
    struct timeval m_tv;
    m_tv.tv_usec = 0; m_tv.tv_sec = 5;
    int cnt = select(read_handle + 2, &readfds, (fd_set *)0, (fd_set *) 0, &m_tv);
    debug(dbg, 0, "read handle: %d\n", cnt);
    int interrupted = (cnt < 0 && errno == EINTR);
    
    debug(dbg, 0, "interrupted: %d cnt: %d\n", interrupted, cnt);
    if (interrupted || cnt == 0) {
      if (pm_check_children(terminated) < 0) break;
    } else if (cnt < 0) {
      perror("select"); 
      exit(9);
    } else if (FD_ISSET (read_handle, &readfds) ) {
      // Read from fin a command sent by Erlang
      int   arity, index, version;
      long  transId;
      char* buf;
      if ((buf = (char *) malloc( BUF_SIZE )) == NULL) return -1;
      /* Reset the index, so that ei functions can decode terms from the 
       * beginning of the buffer */
      index = 0;
      int len = 0;
      if ((len = ei_read(buf, read_handle)) < 0) return -1;
      debug(dbg, 1, "decoding len: %d...\n", len);
      /* Ensure that we are receiving the binary term by reading and 
       * stripping the version byte */
      if (ei_decode_version(buf, &index, &version) < 0) break;
      debug(dbg, 1, "decoding version: %d...\n", version);
      // Decode the tuple header and make sure that the arity is 2
      // as the tuple spec requires it to contain a tuple: {TransId, {Cmd::atom(), Arg1, Arg2, ...}}
      if (ei_decode_tuple_header(buf, &index, &arity) != 2) break;
      debug(dbg, 1, "decoding header arity: %d\n", arity);
      if (ei_decode_long(buf, &index, &transId) < 0) break;
      debug(dbg, 1, "decoding long transId: %d\n", transId);
      if ((arity = ei_decode_tuple_header(buf, &index, &arity)) < 2) break;
      
      fprintf(stderr, "arity: %d\n", arity);
    }
  }
  terminate_all();
  return 0;
}