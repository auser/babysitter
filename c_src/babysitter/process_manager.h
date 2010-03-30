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

#define SIGCHLD_MAX_SIZE 4096

// Signals
int  pm_process_child_signal(pid_t pid);
void pm_pending_signals(int sig);
void pm_gotsignal(int signal);
void pm_gotsigchild(int signal, siginfo_t* si, void* context);
void setup_signal_handlers();

// Returns
extern int handle_ok(int transId, pid_t pid = -1);
extern int handle_pid_status_term(const PidStatusT& stat);
extern int handle_error_str(int transId, bool asAtom, const char* fmt, ...);
extern int handle_pid_list(int transId, const MapChildrenT& children);

// Management
pid_t pm_start_child(const char* command_argv, const char *cd, const char** env, int user, int nice);
int   pm_stop_child(CmdInfo& ci, int transId, time_t &now, bool notify = true);
void  pm_stop_child(pid_t pid, int transId, time_t &now);
int   pm_kill_child(pid_t pid, int signal, int transId, bool notify);
void  pm_terminate_all();
int   pm_check_pending_processes();
int   pm_check_children(int& isTerminated);
int   pm_next_loop();

// Other
void pm_setup();

#endif
