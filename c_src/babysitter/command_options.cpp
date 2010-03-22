#include "command_options.h"

int CmdOptions::ei_decode(ei::Serializer& eis)
{
    // {Cmd::string(), [Option]}
    //      Option = {env, Strings} | {cd, Dir} | {kill, Cmd}
  int sz;
  std::string op, val;
    
  m_err.str("");
  m_nice = INT_MAX;
    
  if (eis.decodeString(m_cmd) < 0) {
    m_err << "badarg: cmd string expected or string size too large";
    return -1;
  } else if ((sz = eis.decodeListSize()) < 0) {
    m_err << "option list expected";
    return -1;
  } else if (sz == 0) {
    m_cd  = "";
    m_kill_cmd = "";
    return 0;
  }

  for ( int i = 0; i < sz; i++) {
    fprintf(stderr, "i: %d\n", i);
    enum OptionT            { CD,   ENV,   KILL,   NICE,   USER,   STDOUT,   STDERR } opt;
    const char* options[] = {"cd", "env", "kill", "nice", "user", "stdout", "stderr"};
      
    if (eis.decodeTupleSize() != 2 || (int)(opt = (OptionT)eis.decodeAtomIndex(options, op)) < 0) {
      m_err << "badarg: cmd option must be an atom"; return -1;
    }
        
    switch (opt) {
      case CD:
      case KILL:
      case USER:
      // {cd, Dir::string()} | {kill, Cmd::string()}
        if (eis.decodeString(val) < 0) {
          m_err << op << " bad option value"; return -1;
        }
        if      (opt == CD)     m_cd        = val;
        else if (opt == KILL)   m_kill_cmd  = val;
        else if (opt == USER) {
          struct passwd *pw = getpwnam(val.c_str());
          if (pw == NULL) {
            m_err << "Invalid user " << val << ": " << ::strerror(errno);
            return -1;
          }
          m_user = pw->pw_uid;
        }
        break;

      case NICE:
        if (eis.decodeInt(m_nice) < 0 || m_nice < -20 || m_nice > 20) {
          m_err << "nice option must be an integer between -20 and 20"; 
          return -1;
        }
        break;

      case ENV: {
        // {env, [NameEqualsValue::string()]}
        int env_sz = eis.decodeListSize();
        if (env_sz < 0) {
          m_err << "env list expected"; return -1;
        } else if ((m_cenv = (const char**) new char* [env_sz+1]) == NULL) {
          m_err << "out of memory"; return -1;
        }

        for (int i=0; i < env_sz; i++) {
          std::string s;
          if (eis.decodeString(s) < 0) {
            m_err << "invalid env argument #" << i; return -1;
          }
          m_env.push_back(s);
          m_cenv[i] = m_env.back().c_str();
        }
        m_cenv[env_sz] = NULL;
        break;
      }

      case STDOUT:
      case STDERR: {
        int type = 0, sz;
        std::string s, fop;
        type = eis.decodeType(sz);
        
        if (type == ERL_ATOM_EXT)
          eis.decodeAtom(s);
        else if (type == ERL_STRING_EXT)
          eis.decodeString(s);
        else if (type == ERL_SMALL_TUPLE_EXT && sz == 2 && 
          eis.decodeAtom(fop) == 0 && eis.decodeString(s) == 0 && fop == "append") {
            ;
        } else {
          m_err << "Atom, string or {'append', Name} tuple required for option " << op;
          return -1;
        }

        std::string& rs = (opt == STDOUT) ? m_stdout : m_stderr;
        std::stringstream ss;
        int fd = (opt == STDOUT) ? 1 : 2;
                
        if (s == "null") {
          ss << fd << ">/dev/null";
          rs = ss.str();
        } else if (s == "stderr" && opt == STDOUT)
          rs = "1>&2";
        else if (s == "stdout" && opt == STDERR)
          rs = "2>&1";
        else if (s != "") {
          ss << fd << (fop == "append" ? ">" : "") << ">\"" << s << "\"";
          rs = ss.str();
        }
        break;
      }
      default:
        m_err << "bad option: " << op; return -1;
      }
    }

    if (m_stdout == "1>&2" && m_stderr != "2>&1") {
      m_err << "cirtular reference of stdout and stderr";
      return -1;
    } else if (!m_stdout.empty() || !m_stderr.empty()) {
      std::stringstream ss;
      ss << m_cmd;
      if (!m_stdout.empty()) ss << " " << m_stdout;
      if (!m_stderr.empty()) ss << " " << m_stderr;
      m_cmd = ss.str();
  }
  return 0;
}
