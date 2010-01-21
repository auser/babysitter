/*---------------------------- Includes ------------------------------------*/
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#include <iomanip>
// System
#include <sys/types.h>
#include <pwd.h>
// Erlang interface
#include "ei++.h"

#include "Honeycomb.h"

/*---------------------------- Implementation ------------------------------*/

using namespace ei;

/**
 * Decode the erlang tuple
 * The erlang tuple decoding happens for all the tuples that get sent over the wire
 * to the c-node.
 **/
int Honeycomb::ei_decode(ei::Serializer& ei) {
  // {Cmd::string(), [Option]}
  //      Option = {env, Strings} | {cd, Dir} | {kill, Cmd}
  int sz;
  std::string op_str, val;
    
  m_err.str("");
  delete [] m_cenv;
  m_cenv = NULL;
  m_env.clear();
  m_nice = INT_MAX;
    
  if (m_eis.decodeString(m_cmd) < 0) {
    m_err << "badarg: cmd string expected or string size too large";
    return -1;
  } else if ((sz = m_eis.decodeListSize()) < 0) {
    m_err << "option list expected";
    return -1;
  } else if (sz == 0) {
    m_cd  = "";
    m_kill_cmd = "";
    return 0;
  }
  
  // Run through the commands and decode them
  enum OptionT            { CD,   ENV,   KILL,   NICE,   USER,   STDOUT,   STDERR,  MOUNT } opt;
  const char* options[] = {"cd", "env", "kill", "nice", "user", "stdout", "stderr", "mount"};
  
  for(int i=0; i < sz; i++) {
    if (m_eis.decodeTupleSize() != 2 || (int)(opt = (OptionT)m_eis.decodeAtomIndex(options, op_str)) < 0) {
      m_err << "badarg: cmd option must be an atom"; 
      return -1;
    }
    printf("Found option: %s\n", op_str.c_str());
    switch(opt) {
      case CD:
      case KILL:
      case USER: {
        // {cd, Dir::string()} | {kill, Cmd::string()} | {user, Cmd::string()} | etc.
        if (m_eis.decodeString(val) < 0) {m_err << opt << " bad option"; return -1;}
        if (opt == CD) {m_cd = val;}
        else if (opt == KILL) {m_kill_cmd = val;}
        else if (opt == USER) {
          struct passwd *pw = getpwnam(val.c_str());
          if (pw == NULL) {m_err << "Invalid user: " << val << " : " << std::strerror(errno); return -1;}
          m_user = pw->pw_uid;
        }
        break;
      }
      case NICE: {
        if (m_eis.decodeInt(m_nice) < 0 || m_nice < -20 || m_nice > 20) {m_err << "Nice must be between -20 and 20"; return -1;}
        break;
      }
      case ENV: {
        int env_sz = m_eis.decodeListSize();
        if (env_sz < 0) {m_err << "Must pass a list for env option"; return -1;}
        else if ((m_cenv = (const char**) new char* [env_sz+1]) == NULL) {
          m_err << "Could not allocate enough memory to create list"; return -1;
        }
        for(int i=0; i < env_sz; i++) {
          std::string str;
          if (m_eis.decodeString(str) >= 0) {
            m_env.push_back(str);
            m_cenv[i] = m_env.back().c_str();
          } else {m_err << "Invalid env argument at " << i; return -1;}
        }
        m_cenv[env_sz] = NULL; // Make sure we have a NULL terminated list
        break;
      }
      case STDOUT:
      case STDERR: {
        int t = 0;
        int sz = 0;
        std::string str, fop;
        t = m_eis.decodeType(sz);
        if (t == ERL_ATOM_EXT) m_eis.decodeAtom(str);
        else if (t == ERL_STRING_EXT) m_eis.decodeString(str);
        else {
          m_err << "Atom or string tuple required for " << op_str;
          return -1;
        }
        // Setup the writer
        std::string& rs = (opt == STDOUT) ? m_stdout : m_stderr;
        std::stringstream stream;
        int fd = (opt == STDOUT) ? 1 : 2;
        if (str == "null") {stream << fd << ">/dev/null"; rs = stream.str();}
        else if (str == "stderr" && opt == STDOUT) {rs = "1>&2";}
        else if (str == "stdout" && opt == STDERR) {rs = "2>&1";}
        else if (str != "") {stream << fd << ">\"" << str << "\"";rs = stream.str();}
        break;
      }
      default:
        m_err << "bad options: " << op_str; return -1;
    }
  }
  
  if (m_stdout == "1>&2" && m_stderr != "2>&1") {
    m_err << "cirtular reference of stdout and stderr";
    return -1;
  } else if (!m_stdout.empty() || !m_stderr.empty()) {
    std::stringstream stream; stream << m_cmd;
    if (!m_stdout.empty()) stream << " " << m_stdout;
    if (!m_stderr.empty()) stream << " " << m_stderr;
    m_cmd = stream.str();
  }
  
  return 0;
}