#ifndef BABYSITTER_TYPES_H
#define BABYSITTER_TYPES_H

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <string>
#include <map>
#include <deque>

#include "command_info.h"

#ifdef __cplusplus

// Types
typedef int                                 ExitStatusT;
typedef pid_t                               KillCmdPidT;
typedef std::pair<pid_t, ExitStatusT>       PidStatusT;
typedef std::pair<pid_t, CmdInfo>           PidInfoT;
typedef std::map <pid_t, CmdInfo>           MapChildrenT;
typedef std::pair<KillCmdPidT, pid_t>       KillPidStatusT;
typedef std::map <KillCmdPidT, pid_t>       MapKillPidT;
typedef std::deque<PidStatusT>              PidStatusDequeT;

#endif

#endif