#ifndef BABYSITTER_H
#define BABYSITTER_H

// Core includes
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <setjmp.h>

#include <deque>

#include "honeycomb.h"
#include "babysitter_utils.h"

#ifndef BABYSITTER_VERSION
// TODO: Make this read from the VERSION file at root
#define BABYSITTER_VERSION 0.1
#endif

#ifndef SIGCHLD_MAX_SIZE
#define SIGCHLD_MAX_SIZE 4096
#endif

//---
// Variables
//---
extern std::deque<PidStatusT> exited_children;
extern sigjmp_buf  jbuf;                   // Jump buffer
extern bool oktojump;
extern bool pipe_valid;      // Is the pip still valid?
extern int  terminated;         // indicates that we got a SIGINT / SIGTERM event
extern long int dbg;        // Debug flag
extern bool signaled;     // indicates that SIGCHLD was signaled

//---
// Functions
//---
void  setup_defaults();
void  setup_signal_handlers();
void  gotsigchild(int signal, siginfo_t* si, void* context);
void  gotsignal(int sig);
int   process_child_signal(pid_t pid);
void  check_pending();

#endif