/*---------------------------- Includes ------------------------------------*/
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#include <iomanip>
#include <assert.h>
#include <fcntl.h>
#include <gelf.h>
#include <regex.h>
// System
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/resource.h>
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
  setup_defaults();
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
  if (!dont_chroot) {
    pid_t child = fork();
    
    if(0 == child) {
      temp_drop(); // drop into new user

      // Find the binary command
      std::string binary_path = find_binary(m_cmd);
      std::string pth = m_cd + binary_path;
      
      // Currently, we only support running binaries, not shell scripts
      copy_deps(binary_path);
      
      // come back to our permissions
      restore_perms();
      
      // Chroot
      // Secure the chroot first
      unsigned int fd, fd_max = 1023;
      struct stat buf; struct rlimit lim;
      if (! getrlimit(RLIMIT_NOFILE, &lim) && (fd_max < lim.rlim_max)) fd_max = lim.rlim_max;
      
      for (fd=0;fd < fd_max; fd++) {
        if ( !fstat(fd, &buf) && S_ISDIR(buf.st_mode))
          if (close(fd)) {
            fprintf(stderr, "Could not close insecure directory/file: %i before chrooting\n", fd);
            exit(-1);
          }
      }
      
      // Chroot please
      if (chroot(m_cd.c_str())) {
        fprintf(stderr, "Could not chroot into %s\n", m_cd.c_str());
        exit(-1);
      }
      // Change directory
      if (chdir(m_cd.c_str())) {
        fprintf(stderr, "Could not chdir into %s after chrooting\n", m_cd.c_str());
        exit(-1);
      }
      
      // Set resource limitations
      // TODO: Make extensible
      temp_drop();
      set_rlimits();
      restore_perms();
      exit(0); // Exit from the child pid
    }
  } else {
    // Cd into the working directory directory
    chdir(m_cd.c_str());
  }
  
  // Success!
  return 0;
}

pid_t Honeycomb::execute() {
  pid_t chld = fork();
  
  // We are in the child pid
  perm_drop(); // Drop into new user forever!!!!
  
  if(chld) {
    // Build the environment vars
    const std::string shell = getenv("SHELL");
    const std::string shell_args = "-c";
    const char* argv[] = { shell.c_str(), shell_args.c_str(), m_cmd.c_str() };
    
    if (execve(m_cmd.c_str(), (char* const*)argv, (char* const*)m_cenv) < 0) {
      fprintf(stderr, "Cannot execute '%s'", m_cmd.c_str());
      return EXIT_FAILURE;
    }
  } else {
    fprintf(stderr, "Could not fork into new process :(\n");
    exit(-1);
  }
  
  if (m_nice != INT_MAX && setpriority(PRIO_PROCESS, chld, m_nice) < 0) {
    fprintf(stderr, "Cannot set priority of pid %d", chld);
    exit(-1);
  }
  
  return chld;
}

void Honeycomb::set_rlimits() {
  if(m_nofiles) set_rlimit(RLIMIT_NOFILE, m_nofiles);
}

void Honeycomb::set_rlimit(const int res, const rlim_t limit) {
  struct rlimit lmt = { limit, limit };
  if (setrlimit(res, &lmt)) {
    fprintf(stderr, "Could not set resource limit: %d\n", res);
    exit(-1);
  }
}

/*---------------------------- UTILS ------------------------------------*/
std::string Honeycomb::find_binary(const std::string& file) {
  assert(geteuid() != 0 && getegid() != 0); // We can't be root to run this.
  
  if (abs_path(file)) return file;
  
  std::string pth = DEFAULT_PATH;
  char *p2 = getenv("PATH");
  if (p2) {pth = p2;}
  
  std::string::size_type i = 0;
  std::string::size_type f = pth.find(":", i);
  do {
    std::string s = pth.substr(i, f - i) + "/" + file;
    if (0 == access(s.c_str(), X_OK)) {return s;}
  } while(std::string::npos != f);
  
  if (!abs_path(file)) {
    fprintf(stderr, "Could not find the executable %s in the $PATH\n", file.c_str());
    return NULL;
  }
  if (0 == access(file.c_str(), X_OK)) return file;
  
  fprintf(stderr, "Could not find the executable %s in the $PATH\n", file.c_str());
  return NULL;
}

bool Honeycomb::abs_path(const std::string & path) {
  return '/' == path[0] || ('.' == path[0] && '/' == path[1]);
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

void Honeycomb::copy_deps(const std::string & image) {
  if (EV_NONE == elf_version(EV_CURRENT)) {
    fprintf(stderr, "ELF libs failed to initialize\n");
    exit(-1);
  }
  int fl = open(image.c_str(), O_RDONLY);
  if (-1 == fl) {
    fprintf(stderr, "Could not open %s - %s\n", image.c_str(), std::strerror(errno));
    exit(-1);
  }
  Elf *elf = elf_begin(fl, ELF_C_READ, NULL);
  if (NULL == elf) {
    fprintf(stderr, "elf_begin failed because %s\n", elf_errmsg(-1));
    exit(-1);
  }
  if (ELF_K_ELF != elf_kind(elf)) {
    fprintf(stderr, "%s is not an ELF object\n", (&image)->c_str());
    exit(-1);
  }
  
  std::pair<string_set *, string_set *> *r = dynamic_loads(elf);
  string_set libs = *r->first;
  
  // Go through the libs
  for (string_set::iterator ld = libs.begin(); ld != libs.end(); ++ld) {
    string_set pths = *r->second;
    bool found = false;
    
    for (string_set::iterator pth = pths.begin(); pth != pths.end(); ++pth) {
      std::string src = *pth + '/' + *ld;
      if (m_already_copied.count(src)) {
        found = true;
        continue;
      }
      std::string dest = m_cd + src;
      
      found = true;
      m_already_copied.insert(src);
      copy_deps(src);
      break;
      
      if (!found) {fprintf(stderr, "Could not find the library %s\n", ld->c_str());}
    }
  }
  
  elf_end(elf);
  close(fl);
}

/**
 * dynamic_loads (from line 443 of isolate.cpp)
 **/
std::pair<string_set *, string_set *> * Honeycomb::dynamic_loads(Elf *elf) {
  assert(geteuid() != 0 && getegid() != 0);

  GElf_Ehdr ehdr;
  if (! gelf_getehdr(elf, &ehdr)) {
    fprintf(stderr, "elf_getehdr failed from %s\n", elf_errmsg(-1));
    exit(-1);
  }

  Elf_Scn *scn = elf_nextscn(elf, NULL);
  GElf_Shdr shdr;
  std::string to_find = ".dynstr";
  while (scn) {
    if (NULL == gelf_getshdr(scn, &shdr)) {
      fprintf(stderr, "getshdr() failed from %s\n", elf_errmsg(-1));
      exit(-1);
    }

    char * nm = elf_strptr(elf, ehdr.e_shstrndx, shdr.sh_name);
    if (NULL == nm) {
      fprintf(stderr, "elf_strptr() failed from %s\n", elf_errmsg(-1));
      exit(-1);
    }

    if (to_find == nm) {
      break;
    }

    scn = elf_nextscn(elf, scn);
  }

  Elf_Data *data = NULL;
  size_t n = 0;
  string_set *lbrrs = new string_set(), *pths = new string_set();

  pths->insert("/lib");
  pths->insert("/usr/lib");
  pths->insert("/usr/local/lib");

  while (n < shdr.sh_size && (data = elf_getdata(scn, data)) ) {
    char *bfr = static_cast<char *>(data->d_buf);
    char *p = bfr + 1;
    while (p < bfr + data->d_size) {
      if (names_library(p)) {
        lbrrs->insert(p);
      } else if ('/' == *p) {
        pths->insert(p);
      }

      size_t lngth = strlen(p) + 1;
      n += lngth;
      p += lngth;
    }
  }

  return new std::pair<string_set *, string_set *>(lbrrs, pths);
}

bool Honeycomb::names_library(const std::string & name) {
  return matches_pattern(name, "^lib.+\\.so[.0-9]*$", 0);
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

/*---------------------------- Permissions ------------------------------------*/

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

bool Honeycomb::matches_pattern(const std::string & matchee, const char * pattern, int flags) {
  regex_t xprsn;
  if (regcomp(&xprsn, pattern, flags|REG_EXTENDED|REG_NOSUB)) {
    fprintf(stderr, "Failed to compile regex\n");
    exit(-1); // we are in the fork
  }

  if (0 == regexec(&xprsn, matchee.c_str(), 1, NULL, 0)) {
    regfree(&xprsn);
    return true;
  }

  regfree(&xprsn);
  return false;
}