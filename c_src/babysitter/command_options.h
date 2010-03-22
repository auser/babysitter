#ifndef COMMAND_OPTIONS_H
#define COMMAND_OPTIONS_H

#include <string.h>
#include <string>
#include <sys/time.h> // For timeval
#include <signal.h>
#include <sstream>
#include <list>
#include <pwd.h>

#include <ei.h>
#include "ei++.h"

class CmdOptions {
  ei::StringBuffer<256>   m_tmp;
  std::stringstream       m_err;
  std::string             m_cmd;
  std::string             m_cd;
  std::string             m_stdout;
  std::string             m_stderr;
  std::string             m_kill_cmd;
  std::list<std::string>  m_env;
  long                    m_nice;     // niceness level
  size_t                  m_size;
  size_t                  m_count;
  int                     m_user;     // run as
  const char**            m_cenv;

public:

  CmdOptions() : m_tmp(0, 256), m_nice(INT_MAX), m_size(0), m_count(0), m_user(INT_MAX), m_cenv(NULL) {
    m_env.push_back("CONTAINER=beehive");
  }
  ~CmdOptions() { delete [] m_cenv; m_cenv = NULL; }

  const char*  strerror() const { return m_err.str().c_str(); }
  const char*  cmd()      const { return m_cmd.c_str(); }
  const char*  cd()       const { return m_cd.c_str(); }
  char* const* env()      const { return (char* const*)m_cenv; }
  const char*  kill_cmd() const { return m_kill_cmd.c_str(); }
  int          user()     const { return m_user; }
  int          nice()     const { return m_nice; }
    
  int ei_decode(ei::Serializer& ei);
};


#endif