
      const string DEV_RANDOM = "/dev/urandom";
      const string LD_ELF_SO_PATH = "/lib/ld-linux.so.2";
      
/* GNU libc present */

#define has_glibc 1

const string NSS_COMPAT="/lib/libnss_compat.so.2";
const string NSS_DNS="/lib/libnss_dns.so.2";
const string NSS_FILES="/lib/libnss_files.so.2";
const string LIBRESOLV = "/lib/libresolv.so.2";
const string NSSWITCH_CONF = "/etc/nsswitch.conf";


const static bool DEBUG = true;
const string DEFAULT_CONFINEMENT_ROOT = "/var/isolation";
string CONFINEMENT_ROOT;
const mode_t CONFINEMENT_ROOT_MODE = 040755;
const string DEFAULT_PATH = "/bin:/usr/bin:/usr/local/bin";
const string RESOLV_CONF = "/etc/resolv.conf";
const string TERMCAP = "/usr/share/misc/termcap.db";
const rlim_t DEFAULT_MEMORY_LIMIT = 0x2000000;

const string CHOWN = "/bin/chown";
const string CP = "/bin/cp";
const string MOUNT = "/bin/mount";
const string RM = "/bin/rm";
const string RMDIR = "/bin/rmdir";
const string UMOUNT = "/bin/umount";
const string XAUTH = "/usr/bin/xauth";
