#ifndef COMMAND_INFO_H
#define COMMAND_INFO_H

#include <string.h>
#include <string>
#include <sys/time.h> // For timeval
#include <signal.h>

/// Contains run-time info of a child OS process.
/// When a user provides a custom command to kill a process this 
/// structure will contain its run-time information.
struct CmdInfo {
  std::string     m_cmd;            // Executed command
  pid_t           m_cmd_pid;        // Pid of the custom kill command
  std::string     m_kill_cmd;       // Kill command to use (if provided - otherwise use SIGTERM)
  pid_t           m_kill_cmd_pid;   // Pid of the command that <pid> is supposed to kill
  time_t          m_deadline;       // Time when the <cmd_pid> is supposed to be killed using SIGTERM.
  bool            m_sigterm;        // <true> if sigterm was issued.
  bool            m_sigkill;        // <true> if sigkill was issued.

  CmdInfo() : m_cmd_pid(-1), m_kill_cmd_pid(-1), m_sigterm(false), m_sigkill(false) {}
  CmdInfo(const CmdInfo& ci) {
    new (this) CmdInfo(ci.cmd(), ci.kill_cmd(), ci.cmd_pid());
  }
  CmdInfo(const char* _cmd, const char* _kill_cmd, pid_t _cmd_pid) {
    new (this) CmdInfo();
    m_cmd      = _cmd;
    m_cmd_pid  = _cmd_pid;
    m_kill_cmd = _kill_cmd;
  }
  
  // Getters
  const char*   cmd()           const {return m_cmd.c_str();}
  const char*   kill_cmd()      const {return m_kill_cmd.c_str(); }
  pid_t         cmd_pid()       const {return m_cmd_pid; }
  pid_t         kill_cmd_pid()  const {return m_kill_cmd_pid;}
  time_t        deadline()      const {return m_deadline;}
  bool          sigterm()       const {return m_sigterm;}
  bool          sigkill()       const {return m_sigkill;}
  
  // Setters
  void set_deadline(time_t t)         {m_deadline = t;}
  void set_sigkill(bool t)            {m_sigkill = t;}
  void set_sigterm(bool t)            {m_sigterm = t;}
  void set_kill_cmd_pid(pid_t p)      {m_kill_cmd_pid = p;}
  
  // functions
  const char* status();
};

#endif