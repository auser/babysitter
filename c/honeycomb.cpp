/*---------------------------- Includes ------------------------------------*/
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#include <iomanip>
#include <assert.h>
// System
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>
// Erlang interface
#include "ei++.h"

#include "Honeycomb.h"

/*---------------------------- Implementation ------------------------------*/

using namespace ei;

int Honeycomb::setup_defaults() {
  /* Setup environment defaults */
  const char* default_env_vars[] = {
   "LD_LIBRARY_PATH=/lib;/usr/lib;/usr/local/lib", 
   "HOME=/mnt"
  };
  
  int m_cenv_c = 0;
  const int max_env_vars = 1000;
  
  if ((m_cenv = (const char**) new char* [max_env_vars]) == NULL) {
    m_err << "Could not allocate enough memory to create list"; return -1;
  }
  
  memcpy(m_cenv, default_env_vars, (std::min((int)max_env_vars, (int)sizeof(default_env_vars)) * sizeof(char *)));
  m_cenv_c = sizeof(default_env_vars) / sizeof(char *);
  
  return 0;
}

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
        
        for(int i=0; i < env_sz; i++) {
          std::string str;
          if (m_eis.decodeString(str) >= 0) {
            m_env.push_back(str);
            m_cenv[m_cenv_c+i] = m_env.back().c_str();
          } else {m_err << "Invalid env argument at " << i; return -1;}
        }
        m_cenv[m_cenv_c+1] = NULL; // Make sure we have a NULL terminated list
        m_cenv_c = env_sz+m_cenv_c; // save the new value... we don't really need to do this, though
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

int Honeycomb::build_environment(std::string confinement_root, mode_t confinement_mode = 040755) {
  /* Prepare as of the environment from the child process */
  // First, get a random_uid to run as
  m_user = random_uid();
  gid_t egid = getegid();
  
  // Create the root directory, the 'cd' if it's not specified
  if (m_cmd.empty()) {
    struct stat stt;
    if (0 == stat(confinement_root.c_str(), &stt)) {
      if (0 != stt.st_uid || 0 != stt.st_gid) {
        m_err << confinement_root << " is not owned by root (0:0). Exiting..."; return -1;
      }
    } else if (ENOENT == errno) {
      setegid(0);
    
      if(mkdir(confinement_root.c_str(), confinement_mode)) {
        m_err << "Could not create the directory " << confinement_root; return -1;
      }
    } else {
      m_err << "Unknown error: " << std::strerror(errno); return -1;
    }
    // The m_cd path is the confinement_root plus the user's uid
    m_cd = confinement_root + "/" + to_string(m_user, 10);
  
    if (mkdir(m_cd.c_str(), confinement_mode)) {
      m_err << "Could not create the new confinement root " << m_cd;
    }
    
    if (chown(m_cd.c_str(), m_user, m_user)) {
      m_err << "Could not chown to the effective user: " << m_user; return -1;
    } 
    setegid(egid); // Return to be the effective user
  }
  
  // Next, create the chroot, if necessary which sets up a chroot
  // environment in the confinement_root path
  if (do_chroot) {
    temp_drop(); // drop into new user
    string_set pths;
    pths.insert("/tmp");
    pths.insert("/bin");
    pths.insert("/lib");
    pths.insert("/libexec");
    pths.insert("/etc");
    // Make these directories please
    for (string_set::iterator i = pths.begin(); i != pths.end(); ++i) {
      if (mkdir(m_cd.c_str(), confinement_mode)) {
        m_err << "Could not build confinement directory: " << std::strerror(errno);
        exit(1);
      }
    }
    
    // Find the binary command
    std::string binary_path = find_binary(m_cmd);
    std::string pth = m_cd + binary_path;
    
    cp(binary_path, pth);
    
  }
  
  // Success!
  return 0;
}

pid_t Honeycomb::run() {
  pid_t pid = fork();
  if (pid < 0) {
    m_err << "Could not launch new pid to start a new environment";
    return -1;
  } else {
    // We are in the child pid
  }
  
  return 0;
}

/*---------------------------- UTILS ------------------------------------*/
std::string Honeycomb::find_binary(const std::string& file) {
  assert(geteuid() != 0 && getegid() != 0); // We can't be root to run this.
  
  if ('/' == file[0] || ('.' == file[0] && '/' == file[1])) return file;
  std::string pth = DEFAULT_PATH;
  char *p2 = getenv("PATH");
  if (p2) {pth = p2;}
  
  std::string::size_type i = 0;
  std::string::size_type f = pth.find(":", i);
  do {
    std::string s = pth.substr(i, f - i) + "/" + file;
    if (0 == access(s.c_str(), X_OK)) {return s;}
  } while(std::string::npos != f);
  if (!('/' == file[0] && ('.' == file[0] && '/' == file[1]))) {
    fprintf(stderr, "Could not find the executable %s in the $PATH\n", file.c_str());
    return NULL;
  }
  if (0 == access(file.c_str(), X_OK)) return file;
  fprintf(stderr, "Could not find the executable %s in the $PATH\n", file.c_str());
  return NULL;
}

void Honeycomb::make_path(const std::string & path) {
  struct stat stt;
  if (0 == stat(path.c_str(), &stt) && S_ISDIR(stt.st_mode)) return;

  std::string::size_type lngth = path.length();
  for (std::string::size_type i = 1; i < lngth; ++i) {
    if (lngth - 1 == i || '/' == path[i]) {
      std::string p = path.substr(0, i + 1);
      if (mkdir(p.c_str(), 0750) && EEXIST != errno && EISDIR != errno) {
        fprintf(stderr, "Could not create the directory %s", p.c_str());
        exit(-1);
      }
    }
  }
}

void Honeycomb::cp(std::string & source, std::string & destination) {
  struct stat stt;
  if (0 == stat(destination.c_str(), &stt)) {return;}

  FILE *src = fopen(source.c_str(), "rb");
  if (NULL == src) { exit(-1);}

  FILE *dstntn = fopen(destination.c_str(), "wb");
  if (NULL == dstntn) {
    fclose(src);
    exit(-1);
  }

  char bfr [4096];
  while (true) {
    size_t r = fread(bfr, 1, sizeof bfr, src);
      if (0 == r || r != fwrite(bfr, 1, r, dstntn)) {
        break;
      }
    }

  fclose(src);
  if (fclose(dstntn)) {
    exit(-1);
  }
}


const char *DEV_RANDOM = "/dev/urandom";
uid_t Honeycomb::random_uid() {
  uid_t u;
  for (unsigned char i = 0; i < 10; i++) {
    int rndm = open(DEV_RANDOM, O_RDONLY);
    if (sizeof(u) != read(rndm, reinterpret_cast<char *>(&u), sizeof(u))) {
      continue;
    }
    close(rndm);

    if (u > 0xFFFF) {
      return u;
    }
  }

  printf("Could not generate a good random UID after 10 attempts. Bummer!");
  exit(-1);
}
const char * const Honeycomb::to_string(long long int n, unsigned char base) {
  static char bfr[32];
  const char * frmt = "%lld";
  if (16 == base) {frmt = "%llx";} else if (8 == base) {frmt = "%llo";}
  snprintf(bfr, sizeof(bfr) / sizeof(bfr[0]), frmt, n);
  return bfr;
}

void Honeycomb::temp_drop() {
  if (setresgid(-1, m_user, getegid()) || setresuid(-1, m_user, geteuid()) || getegid() != m_user || geteuid() != m_user ) {
    fprintf(stderr, "Could not drop privileges temporarily to %d: %s\n", m_user, std::strerror(errno));
    exit(-1); // we are in the fork
  }
}
void Honeycomb::perm_drop() {
  uid_t ruid, euid, suid;
  gid_t rgid, egid, sgid;

  if (setresgid(m_user, m_user, m_user) || setresuid(m_user, m_user, m_user) || getresgid(&rgid, &egid, &sgid)
      || getresuid(&ruid, &euid, &suid) || rgid != m_user || egid != m_user || sgid != m_user
      || ruid != m_user || euid != m_user || suid != m_user || getegid() != m_user || geteuid() != m_user ) {
    fprintf(stderr,"\nError setting user to %u\n",m_user);
    err.write("Could not drop down");
    exit(-1);
  }
}
void Honeycomb::restore_perms() {
  uid_t ruid, euid, suid;
  gid_t rgid, egid, sgid;

  if (getresgid(&rgid, &egid, &sgid) || getresuid(&ruid, &euid, &suid) || setresuid(-1, suid, -1) || setresgid(-1, sgid, -1)
      || geteuid() != suid || getegid() != sgid ) {
        fprintf(stderr, "Could not drop privileges temporarily to %d: %s\n", m_user, std::strerror(errno));
        exit(-1); // we are in the fork
      }
}