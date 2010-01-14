/**
 * babysitter -- Mount and run a command in a chrooted environment
 * with an image
 *
 * Based on the brilliant work of snackypants, aka Chris Palmer <chris@isecpartners.com>.
 */


#include <sys/param.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mount.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <gelf.h>
#ifdef linux
  #include <grp.h>    // For setgroups(2)
#endif
#include <libgen.h>
#include <regex.h>
#include <signal.h>

#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <stdexcept>
#include <dirent.h>

using namespace std;

#include "help.h"
#include "privilege.h"
#include "babysitter.h"
#include "ei++.h"

typedef set<string> string_set;

/* Functions. */
static uid_t random_uid() throw (runtime_error);
/**
 * NOTE: The implementation of this function is amusing.
 *
 * @param n The number to convert to a string.
 *
 * @param base The base in which to represent the number. Can be 8, 10, or
 * 16. Any other value is treated as if it were 10.
 *
 * @return A pointer to a static array of characters representing the number
 * as a string.
 */
static const char * const to_string(long long int n, unsigned char base) {
  static char bfr[32];

  const char * frmt = "%lld";
  if (16 == base) {
    frmt = "%llx";
  } else if (8 == base) {
    frmt = "%llo";
  }
  snprintf(bfr, sizeof(bfr) / sizeof(bfr[0]), frmt, n);
  return bfr;
}

/**
 * @param source The name of the source file.
 * @param destination The name of the destination file.
 *
 * @throws runtime_error If the file could not be copied for any reason.
 */
static void copy_file(const string &source, const string &destination) throw (runtime_error) {
  struct stat stt;
  if (0 == stat(destination.c_str(), &stt))
    return;

  FILE *src = fopen(source.c_str(), "rb");
  if (NULL == src)
    throw runtime_error(string("Could not read ") + source + ": " + strerror(errno));

  FILE *dstntn = fopen(destination.c_str(), "wb");
  if (NULL == dstntn) {
    fclose(src);
    throw runtime_error(string("Could not write ") + destination + ": " + strerror(errno));
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
    throw runtime_error(string("Could not write ") + destination + ": " + strerror(errno));
  }
}

/**
 * Check if it's an absolute path
 **/
static bool is_absolute_path(const string & path) throw () {
  return '/' == path[0] || ('.' == path[0] && '/' == path[1]);
}

/**
 * @param file The basename of a file to find in one of the directories
 * specified in $PATH. (If $PATH is not available, DEFAULT_PATH is used.)
 *
 * @return A new string giving the full path in which file was found.
 *
 * @throws range_error if file was not found in $PATH/DEFAULT_PATH.
 */
const string DEFAULT_PATH = "/bin:/usr/bin:/usr/local/bin";
static string which(const string &file) throw (runtime_error) {
  if (is_absolute_path(file))
    return file;

  string pth = DEFAULT_PATH;
  char *p = getenv("PATH");
  if (p)
    pth = p;

  string::size_type i = 0;
  string::size_type f = pth.find(':', i);
  do {
    string s = pth.substr(i, f - i) + "/" + file;
    if (0 == access(s.c_str(), X_OK))
      return s;

    i = f + 1;
    f = pth.find(':', i);
  } while (string::npos != f);

  if (! is_absolute_path(file))
    throw runtime_error("File not found in $PATH");

  if (0 == access(file.c_str(), X_OK))
    return file;

  throw runtime_error("File not found in $PATH");
}

/**
 * @param matchee The string to match against pattern.
 * @param pattern The pattern to test.
 * @param flags The regular expression compilation flags (see regex(3)).
 * (REG_EXTENDED|REG_NOSUB is applied implicitly.)
 *
 * @return true if matchee matches pattern, false otherwise.
 */
static bool matches_pattern(const string &matchee, const char * pattern, int flags) throw (logic_error)
{
  regex_t xprsn;
  if (regcomp(&xprsn, pattern, flags|REG_EXTENDED|REG_NOSUB))
    throw logic_error("Failed to compile regular expression");

  if (0 == regexec(&xprsn, matchee.c_str(), 1, NULL, 0)) {
        regfree(&xprsn);
        return true;
  }

  regfree(&xprsn);
  return false;
}

/**
 * @param path The path to create, with successive calls to mkdir(2).
 *
 * @throws runtime_error If any call to mkdir failed.
 */
static void make_path(const string & path) {
  struct stat stt;
  if (0 == stat(path.c_str(), &stt) && S_ISDIR(stt.st_mode))
    return;

  string::size_type lngth = path.length();
  for (string::size_type i = 1; i < lngth; ++i) {
    if (lngth - 1 == i || '/' == path[i]) {
      string p = path.substr(0, i + 1);
      if (mkdir(p.c_str(), 0750) && EEXIST != errno && EISDIR != errno) {
            throw runtime_error("Could not create path (make_path:230) " + p + ": " + strerror(errno));
      }
    }
  }
}

/**
 * Create the confinement root
 **/
static void create_confinement_root(const string &confinement_root, const mode_t confinement_root_mode) throw (runtime_error) {
  struct stat stt;

  if (0 == stat(confinement_root.c_str(), &stt)) {
    if (0 != stt.st_uid || 0 != stt.st_gid) {
      throw runtime_error(confinement_root + " not owned by 0:0. Cannot continue.");
    }

    if (stt.st_mode != confinement_root_mode) {
      throw runtime_error(confinement_root + " is mode " + to_string(stt.st_mode, 8) + 
        "; should be " + to_string(confinement_root_mode, 8));
    }
  } else if (ENOENT == errno) {
    gid_t egid = getegid();
    setegid(0);
    
    if (mkdir(confinement_root.c_str(), confinement_root_mode)) {
      throw runtime_error("Could not create confinement_root" + confinement_root + ":" + strerror(errno));
    }  
    
    setegid(egid);
  }
  else {
    throw runtime_error(confinement_root + ": " + strerror(errno));
  }
}

/**
 * Add nice error messaging
 **/
static map<int, string> populate_resources() {
  static map<int, string> resources;
  resources[RLIMIT_AS]      = "RLIMIT_AS (virtual memory size)";
  resources[RLIMIT_CORE]    = "RLIMIT_CORE (core file size)";
  resources[RLIMIT_DATA]    = "RLIMIT_DATA (data segment size)";
  resources[RLIMIT_NOFILE]  = "RLIMIT_NOFILE (number of files)";
  resources[RLIMIT_MEMLOCK] = "RLIMIT_MEMLOCK (locked memory size)";
  resources[RLIMIT_NPROC]   = "RLIMIT_NPROC (simultaneous processes)";
  resources[RLIMIT_RSS]     = "RLIMIT_RSS (resident set size)";
  resources[RLIMIT_STACK]   = "RLIMIT_STACK (stack size)";
  resources[RLIMIT_CPU]     = "RLIMIT_CPU (CPU time)";
  resources[RLIMIT_FSIZE]   = "RLIMIT_FSIZE (file size)";

  return resources;
}


/**
 * @param resource The resource to set the limit on. (See setrlimit(2).)
 *
 * @param limit The value -- hard and soft -- of the limit.
 *
 * @throws runtime_error If setrlimit fails.
 */
static void set_resource_limit(const int resource, const rlim_t limit) throw (runtime_error) {
  struct rlimit rlmt = { limit, limit };
  if (setrlimit(resource, &rlmt)) {
    static map<int, string> resources = populate_resources();
    throw runtime_error("Could not set resource " + resources[resource] + 
      " to " + to_string(limit, 10) + ": " + strerror(errno));
  }
}

/**
 * Confines the process to confinement_path.
 *
 * @throws runtime_error If chroot(2) or chdir(2) fail.
 */
static void confine(string confinement_path) throw (runtime_error) {
  if (chdir(confinement_path.c_str()) || chroot(confinement_path.c_str()) || chdir("/")) {
    throw runtime_error("Could not confine to " + confinement_path + ": " + strerror(errno));
  }
}

/**
 * @return An unpredictable (uid_t is not large enough for me to say
 * "secure") UID.
 *
 * @throws runtime_error If a security assertion cannot be met.
 */
const string DEV_RANDOM = "/dev/urandom";
static uid_t random_uid() throw (runtime_error) {
  uid_t u;
  for (unsigned char i = 0; i < 10; i++) {
    int rndm = open(DEV_RANDOM.c_str(), O_RDONLY);
    if (sizeof(u) != read(rndm, reinterpret_cast<char *>(&u), sizeof(u))) {
      continue;
    }
    close(rndm);

    if (u > 0xFFFF) {
      return u;
    }
  }

  throw runtime_error("Could not generate a good random UID after 10 attempts. Bummer!");
}

/**
 * @param file The file to test.
 *
 * @return True if the file's first two bytes are "#!". Advances the file
 * pointer by two bytes.
 */
static bool shell_magic(FILE *file) throw () {
  char bfr[2];
  if (2 != fread(bfr, sizeof(char), 2, file)) {
    return false;
  }

  if ('#' == bfr[0] && '!' == bfr[1]) {
    return true;
  }

  return false;
}

/**
 * @param name A string that might be the name of a code library, e.g.
 * libc.so.7.
 *
 * @return Whether or not the string is plausibly the name of a code
 * library.
 */
static bool names_library(const string & name) throw() {
 return matches_pattern(name, "^lib.+\\.so[.0-9]*$", 0);
}

/**
 * @param elf A pointer to an Elf object (see elf(3)).
 *
 * @return A pair of vectors of strings. The first member is a vector of
 * libraries (e.g. "libc.so.7"), and the second is a vector of search paths
 * (e.g. "/usr/local/gadget/lib").
 *
 * @throws runtime_error If the Elf could not be parsed normally.
 */
static pair<string_set *, string_set *> * dynamic_loads(Elf *elf) throw (runtime_error) {
  assert(geteuid() != 0 && getegid() != 0);

  GElf_Ehdr ehdr;
  if (! gelf_getehdr(elf, &ehdr)) {
    throw runtime_error(string("elf_getehdr failed: ") + elf_errmsg(-1));
  }

  Elf_Scn *scn = elf_nextscn(elf, NULL);
  GElf_Shdr shdr;
  string to_find = ".dynstr";
  while (scn) {
    if (NULL == gelf_getshdr(scn, &shdr)) {
      throw runtime_error(string("getshdr() failed: ") + elf_errmsg(-1));
    }
    char * nm = elf_strptr(elf, ehdr.e_shstrndx, shdr.sh_name);
    if (NULL == nm) {
      throw runtime_error(string("elf_strptr() failed: ") + elf_errmsg(-1));
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

  return new pair<string_set *, string_set *>(lbrrs, pths);
}

/**
 * @param image The path to the executable image file.
 *
 * @return The number of dependencies copied into confinement_path.  XXX:
 * This allows a malicious isolatee to increase the number of files it can
 * open. However, I think it is all under the control of the loader?
 *
 * @throws runtime_error if the image could not be parsed. Does not throw
 * if a dependency could not be copied.
 */
string_set already_copied;
static rlim_t copy_dependencies(const string image, const string confinement_path) throw (runtime_error) {
  if (EV_NONE == elf_version(EV_CURRENT)) {
    throw runtime_error(string("ELF library initialization failed: ") + elf_errmsg(-1));
  }

  int fl = open(image.c_str(), O_RDONLY);
  if (-1 == fl) {
    throw runtime_error(string("Could not open ") + image + ": " + strerror(errno));
  }

  Elf *elf = elf_begin(fl, ELF_C_READ, NULL);
  if (NULL == elf) {
    throw runtime_error(string("elf_begin failed: ") + elf_errmsg(-1));
  }

  if (ELF_K_ELF != elf_kind(elf)) {
    throw runtime_error(image + " is not an ELF object.");
  }

  pair<string_set *, string_set *> *r = dynamic_loads(elf);
  string_set lds = *r->first;
  rlim_t lmt_fls = 0;

  for (string_set::iterator ld = lds.begin(); ld != lds.end(); ++ld) {
    string_set pths = *r->second;
    bool fnd = false;

    for (string_set::iterator pth = pths.begin(); pth != pths.end(); ++pth) {
      string src = *pth + '/' + *ld;
      if (already_copied.count(src)) {
        fnd = true;
        continue;
      }
      string dstntn = confinement_path + src;

      try {
        make_path(dirname(strdup(dstntn.c_str())));

        copy_file(src, dstntn);
        lmt_fls += 1;
        fnd = true;
        already_copied.insert(src);
        lmt_fls += copy_dependencies(src, confinement_path);    // FIXME: Grossly inefficient.
        break;
      } catch (runtime_error e) { }
    }

    if (! fnd) {
      cerr << "Could not find library " << *ld << endl;
      }
    }

  elf_end(elf);
  close(fl);
  return lmt_fls;
}

/**
 * @param resource The resource for which to find the limit. See
 * getrlimit(2).
 *
 * @return The current process' maximum limit for the given resource.
 */
static rlim_t get_resource_limit(int resource) throw () {
  struct rlimit rlmt;
  if (getrlimit(resource, &rlmt)) {
    throw runtime_error(string("getrlimit: ") + strerror(errno));
  }
  return rlmt.rlim_max;
}

/**
 * For each support path, recursively copies it into confinement_path by
 * calling cp(1) through system(3).
 *
 * @param support_paths The paths to the support directories.
 *
 * @throws runtime_error if the cp command fails for any reason.
 */
static void copy_support_paths(const string_set &support_paths, const string &confinement_path) throw (runtime_error) {
  for (string_set::const_iterator i = support_paths.begin(); i != support_paths.end(); ++i) {
    string src = *i;
    string wildcard ("*");
    size_t found;
    string cmnd;
    
    found = src.find(wildcard);
    
    // Make the paths
    string dstntn_pth = confinement_path + dirname(strdup(src.c_str()));

    if(found != string::npos)
    {
      // We found a wildcard, we'll use a command instead
      make_path(dstntn_pth.c_str());
      cmnd = string("cp -RL") + " " + src + " " + dstntn_pth + "";
      if (system(cmnd.c_str())) {
        throw runtime_error("Could not copy " + src + " into " + confinement_path);
      }
    } else {
      struct stat stt;
      if (stat(src.c_str(), &stt)) {
        throw runtime_error(src + ": " + strerror(errno));
      }

      make_path(dstntn_pth.c_str());

      // If it's just a file, copy it directly and continue.
      if (S_ISREG(stt.st_mode)) {
        copy_file(src, confinement_path + src);
        continue;
      }

      cmnd = string("cp -RL") + " \"" + src + "\" \"" + dstntn_pth + "\"";
      if (system(cmnd.c_str())) {
        throw runtime_error("Could not copy " + src + " into " + confinement_path);
      }
    }
  }
}

/**
 * Is this a directory
 **/
bool is_directory(char path[]) {
  if (path[strlen(path)] == '.') {return true;}
  for (int i = strlen(path) - 1; i >= 0; i--) {
    if (path[i] == '.') return false;
    else if (path[i] == '\\' || path[i] == '/') { return true; }
  }
  return false; // Should never get here
}
/**
 * Recursively delete
 **/
int recursively_delete(string path) {
  if((path[path.length() - 1]) != '\\') path+= "\\";
  
  DIR *pdir = NULL;
  pdir = opendir(path.c_str());
  struct dirent *pent = NULL;
  if(pdir == NULL) {
    return false;
  }
  
  char file[256];
  int counter = 1;
  
  while ( pent = readdir(pdir) ) {
    if(counter > 2) {
      for(int i = 0; i < 256; i++) {
        strcat(file, path.c_str());
      }
      if (pent == NULL) return false;
      strcat(file, pent->d_name);
      if (is_directory(file) == true) { recursively_delete(file); } else { remove(file); }
    } counter ++;
  }
  return 0;
}

/**
 * Performs necessary filesystem clean-up after the isolatee exits.
 */
static void clean_up() {
  // Nothing... yet?
}

void signal_handler(int received) {
  cerr << "Received signal " << received << endl;
  cerr << endl;
  cout << endl;
  _exit(1);
}

/**
 * Installs the signal handlers and the atexit handler to clean up the
 * isolation environment when the parent exits.
 */
void install_handlers() {
  atexit(clean_up);
#ifdef linux
  #define signal bsd_signal
#endif
  signal(SIGHUP, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGQUIT, signal_handler);
  signal(SIGILL, signal_handler);
  signal(SIGTRAP, signal_handler);
  signal(SIGABRT, signal_handler);
  signal(SIGFPE, signal_handler);
  signal(SIGBUS, signal_handler);
  signal(SIGSEGV, signal_handler);
  signal(SIGSYS, signal_handler);
  signal(SIGPIPE, signal_handler);
  signal(SIGALRM, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGTTIN, signal_handler);
  signal(SIGTTOU, signal_handler);
  signal(SIGXCPU, signal_handler);
  signal(SIGXFSZ, signal_handler);
  signal(SIGVTALRM, signal_handler);
  signal(SIGPROF, signal_handler);
  signal(SIGUSR1, signal_handler);
  signal(SIGUSR2, signal_handler);
}

/**
 * Write the contents into the file
 **/
void write_contents_to_file(string file_content, string to_file) {
  ofstream file_ptr(to_file.c_str());
  file_ptr << file_content;
  file_ptr << flush;
  file_ptr.close();
}

const mode_t CONFINEMENT_ROOT_MODE = 040755;
const string RESOLV_CONF = "/etc/resolv.conf";
const string TERMCAP = "/usr/share/misc/termcap.db";
const rlim_t DEFAULT_MEMORY_LIMIT = 0x2000000;

void copy_required_files(uid_t isolator,
                        string fl_pth,
                        const string confinement_path, 
                        const mode_t confinement_root_mode, 
                        string skel_dir) {
  /** 
   * Copy the necessary files into the confinement directory. 
   * If the user provided a skeleton directory, then we can skip
   * all of the smart lookups and just use that directory
   * otherwise, track down the support files and build the 
   * confinement_root
   **/
  string pth = confinement_path + fl_pth;
  if(! skel_dir.empty() )
  {
    string cmnd = string("cp -RL") + " " + skel_dir + "/* " + confinement_path + "";
    if (system(cmnd.c_str())) {
      cerr << "WARNING: Could not build from the skeleton directory" << endl;
    }
  } else {
    string_set pths;
    pths.insert("/tmp");
    pths.insert("/bin");
    pths.insert("/lib");
    pths.insert("/libexec");
    pths.insert("/etc");
    for (string_set::iterator i = pths.begin(); i != pths.end(); ++i) {
      pth = confinement_path + *i;
      if (mkdir(pth.c_str(), confinement_root_mode)) {
        cerr << "Could not build confinement directory: " << strerror(errno) << endl;
        _exit(1);
      }
    }
    
    string_set sprt_pths;
    /* Copy the executable image into the confinement directory. */
    make_path(dirname(strdup(pth.c_str())));
    
    copy_file(fl_pth, pth);

    if (chmod(pth.c_str(), 0755)) {
      cerr << "Could not make " << pth << " executable: " << strerror(errno) << endl;
      _exit(1);
    }

    /* Add a copy of the user's /etc/passwd */
    string contents, fpath;
    
    contents = string("me:x:") + to_string(isolator, 10) + ":" + to_string(isolator, 10) + ":me:/mnt:bin/bash";
    fpath = confinement_path + "/etc/passwd";
    write_contents_to_file(contents, fpath);

    /* Add default /etc/hosts */
    contents = string("127.0.0.1    localhost");
    fpath = confinement_path + "/etc/hosts";
    write_contents_to_file(contents, fpath);

    /* Copy the support directories into the isolation environment. */
    copy_support_paths(sprt_pths, confinement_path);

    // Make this dynamic.. please? thanks
    string ld_elf_so_path = string("/lib/ld-linux.so.2");
    pth = confinement_path + '/' + ld_elf_so_path.c_str();
    copy_file(ld_elf_so_path.c_str(), pth);

#ifdef has_glibc 
    copy_file(NSSWITCH_CONF, confinement_path + NSSWITCH_CONF);
    copy_file(NSS_COMPAT, confinement_path + NSS_COMPAT);
    copy_file(NSS_DNS, confinement_path + NSS_DNS);
    copy_file(NSS_FILES, confinement_path + NSS_FILES);
    copy_file(LIBRESOLV, confinement_path + LIBRESOLV);
#endif
 
    copy_file(RESOLV_CONF, confinement_path + RESOLV_CONF);
  }
}

void load_executable_into_environment(string fl_pth, string confinement_path) {
  /** 
   * Find out if the requested program is a script, and copy in
   * the script stuff. 
   **/
  rlim_t lmt_fls = 0;
  FILE *fl = fopen(fl_pth.c_str(), "rb");
  if (shell_magic(fl)) {
    char pth[PATH_MAX];
    size_t r = fread(pth, sizeof(char), PATH_MAX, fl);
    pth[r] = '\0';

    // Trim whitespace at the beginning and end of the
    // interpreter path.
    char *p = pth;
    while (' ' == *p) {
          p++;
    }
    char *q = strchr(p, '\n');
    if (NULL == q) {
          q = strchr(p, ' ');
    }
    *q = '\0';

    string cp = p;

    string pth2 = confinement_path + dirname(strdup(cp.c_str()));
    
    make_path(pth2);
    pth2 = confinement_path + p;
    copy_file(cp, pth2);
    chmod(pth2.c_str(), 0755);
    lmt_fls += copy_dependencies(cp, confinement_path) + 5;
  } else {
    /* Figure out what the requested program links to, and copy
     * those things into the chroot environment. */
    lmt_fls += copy_dependencies(fl_pth, confinement_path);
  }
  fclose(fl);
}

uid_t build_env(uid_t isolator,
                const string image_path,
                const string filesystemtype,
                const string base_dir) 
{  
  const string confinement_path = base_dir + '/' + to_string(isolator, 16);
  const mode_t confinement_root_mode = 040755;
  create_confinement_root( confinement_path, confinement_root_mode );
  if (chown(confinement_path.c_str(), isolator, isolator)) {
    throw runtime_error("Could not chown confinement_path " + confinement_path + ":" + strerror(errno));
  }
  
  string cmnd;
  
  /**
   * If specified, mount the image file that is given
   **/
   if (!image_path.empty())
   {
     string at = confinement_path + "/mnt";
     
     make_path(at);
     
     cmnd = "mount -o ro -o loop " + image_path + " " + at;     
     if (system(cmnd.c_str()))
       throw runtime_error("Could not mount "+ image_path);
   }
   
   return isolator;
}

pid_t babysit(uid_t isolator, 
            const char* cmd,
            string confinement_path,
            char* const* env,
            ei::StringBuffer<128> err)
{
  // Setup defaults
  rlim_t
    lmt_all = min(DEFAULT_MEMORY_LIMIT, get_resource_limit(RLIMIT_AS)),
    lmt_cr = min(DEFAULT_MEMORY_LIMIT, get_resource_limit(RLIMIT_CORE)),
    lmt_dta = min(DEFAULT_MEMORY_LIMIT, get_resource_limit(RLIMIT_DATA)),
    lmt_fls = min(static_cast<rlim_t>(3), get_resource_limit(RLIMIT_NOFILE)),
    lmt_fl_sz = min(DEFAULT_MEMORY_LIMIT, get_resource_limit(RLIMIT_FSIZE)),
    lmt_mlck = min(static_cast<rlim_t>(0x200000), get_resource_limit(RLIMIT_MEMLOCK)),
    lmt_prcs = min(static_cast<rlim_t>(0), get_resource_limit(RLIMIT_NPROC)),
    lmt_rss = min(DEFAULT_MEMORY_LIMIT, get_resource_limit(RLIMIT_RSS)),
    lmt_stck = min(DEFAULT_MEMORY_LIMIT, get_resource_limit(RLIMIT_STACK)),
    lmt_tm = min(static_cast<rlim_t>(10), get_resource_limit(RLIMIT_CPU));
  /** 
   * Build the default environments
   **/
  uid_t invoker = getegid();
  int curr_env_vars;
  /* Set default environment variables */
  /* We need to set this so that the loader can find libs in /usr/local/lib. */
  const char* default_env_vars[] = {
   "LD_LIBRARY_PATH=/lib;/usr/lib;/usr/local/lib", 
   "HOME=/mnt"
  };
  const int max_env_vars = 1000;
  char *env_vars[max_env_vars];

  memcpy(env_vars, default_env_vars, (min((int)max_env_vars, (int)sizeof(default_env_vars)) * sizeof(char *)));
  curr_env_vars = sizeof(default_env_vars) / sizeof(char *);

  for(size_t i = 0; i < sizeof(env) / sizeof(char *); ++i)
    env_vars[curr_env_vars++] = strdup(env[i]);
  
  pid_t pid = fork();
  if (-1 == pid)
    throw runtime_error("Could not fork");
  
  int sts = 0;
  if (0 == pid) {
    drop_privilege_temporarily(isolator);
    string fl_pth = which(cmd);
    string pth = confinement_path + fl_pth;

    const mode_t confinement_root_mode = 040755; // Redundant... yes, I know
    copy_required_files(isolator, fl_pth, confinement_path, confinement_root_mode, string(""));

    /** 
     * Work with the executable that the user requested to run
     **/
    if (chmod(pth.c_str(), 0755)) {
      cerr << "Could not make " << pth << " executable: " << strerror(errno) << endl;
      _exit(1);
    }

    load_executable_into_environment(fl_pth, confinement_path);

    /* Do the final sandboxing. */
    restore_privilege();
    confine(confinement_path);

    /* Set the resource limits. */
    drop_privilege_temporarily(invoker);
    set_resource_limit(RLIMIT_AS, lmt_all);
    set_resource_limit(RLIMIT_CORE, lmt_cr);
    set_resource_limit(RLIMIT_DATA, lmt_dta);
    //set_resource_limit(RLIMIT_NOFILE, lmt_fls);
    set_resource_limit(RLIMIT_FSIZE, lmt_fl_sz);
    set_resource_limit(RLIMIT_MEMLOCK, lmt_mlck);

    set_resource_limit(RLIMIT_NPROC, lmt_prcs + 1);

    set_resource_limit(RLIMIT_RSS, lmt_rss);
    set_resource_limit(RLIMIT_STACK, lmt_stck);
    set_resource_limit(RLIMIT_CPU, lmt_tm);
    restore_privilege();

    /* Become the isolator, irrevocably. */
    drop_privilege_permanently(isolator);

    /* Set the resource limits to their new minima. Previously,
     * they were set to root's minima (at the top of main), but we
     * do this again here with the new, low-privilege values. */
    lmt_all = min(lmt_all, get_resource_limit(RLIMIT_AS));
    lmt_cr = min(lmt_cr, get_resource_limit(RLIMIT_CORE));
    lmt_dta = min(lmt_dta, get_resource_limit(RLIMIT_DATA));
    lmt_fls = min(lmt_fls, get_resource_limit(RLIMIT_NOFILE));
    lmt_fl_sz = min(lmt_fl_sz, get_resource_limit(RLIMIT_FSIZE));
    lmt_mlck = min(lmt_mlck, get_resource_limit(RLIMIT_MEMLOCK));
    // #ifndef linux
    //     lmt_ntwrk = min(lmt_ntwrk, get_resource_limit(RLIMIT_SBSIZE));
    // #endif
    lmt_prcs = min(lmt_prcs, get_resource_limit(RLIMIT_NPROC));
    lmt_rss = min(lmt_rss, get_resource_limit(RLIMIT_RSS));
    lmt_stck = min(lmt_stck, get_resource_limit(RLIMIT_STACK));
    lmt_tm = min(lmt_tm, get_resource_limit(RLIMIT_CPU));

    /**
     * Execute the sandbox -> fork -> run
     **/
    char* argv[] = {NULL};
    if (execve(fl_pth.c_str(), argv, env_vars)) {
      err.write("Cannot execute chroot");
      return EXIT_FAILURE;
    }
  } else {
    install_handlers();
    
    cerr << "In parent process... I wonder why..." << endl;
    (void) wait(&sts);
  }
  
  return pid;
}