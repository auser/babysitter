#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>             // For timeval struct
#include <time.h>                 // For time function
#include <string>
#include <map>
#include <deque>
#include <pwd.h>        /* getpwdid */

#include "babysitter_types.h"
#include "babysitter_utils.h"
#include "fs.h"
#include "time_utils.h"
#include "print_utils.h"
#include "command_info.h"

// Signals
int process_child_signal(pid_t pid);
void pending_signals(int sig);
void gotsignal(int signal);
void gotsigchild(int signal, siginfo_t* si, void* context);
void setup_signal_handlers();

// Returns
extern int send_ok(int transId, pid_t pid = -1);
extern int send_pid_status_term(const PidStatusT& stat);
extern int send_error_str(int transId, bool asAtom, const char* fmt, ...);
extern int send_pid_list(int transId, const MapChildrenT& children);

// Management
pid_t start_child(int command_argc, const char** command_argv, const char *cd, const char** env, int user, int nice);
int stop_child(CmdInfo& ci, int transId, time_t &now, bool notify = true);
void stop_child(pid_t pid, int transId, time_t &now);
int kill_child(pid_t pid, int signal, int transId, bool notify);
void terminate_all();
void check_pending();
int check_children(int& isTerminated);

// Other
void setup_defaults();

#endif