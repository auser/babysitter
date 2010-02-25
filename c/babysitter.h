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
std::deque<PidStatusT> exited_children;
sigjmp_buf  jbuf;                   // Jump buffer
static bool oktojump   = false;
static bool pipe_valid = true;      // Is the pip still valid?
static int  terminated = 0;         // indicates that we got a SIGINT / SIGTERM event
static long int dbg     = 0;        // Debug flag
static bool signaled   = false;     // indicates that SIGCHLD was signaled

//---
// Functions
//---
void  setup_defaults();
void  setup_signal_handlers(struct sigaction sact, struct sigaction sterm);
void  gotsigchild(int signal, siginfo_t* si, void* context);
void  gotsignal(int sig);
int   process_child_signal(pid_t pid);

#endif