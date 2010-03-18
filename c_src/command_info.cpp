#include "command_info.h"

const char* CmdInfo::status() {
  if (m_sigterm)
    return "terminated";
  else if (m_sigkill)
    return "killed";
  else
    if (kill(m_cmd_pid, 0) == 0) // process likely forked and is alive
      return "running";
    else
      return "stopped";
}