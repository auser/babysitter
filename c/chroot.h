#include <ei.h>
#include "ei++.h"

// Defines
#define SET_RLIMIT (const int resource, const rlim_t lim) \
  struct rlimit rlmt = {lim, lim}; \
  if (setrlimit(resource, &rlmt)) { \
    perror("set resource limit");	\
  	    exit(-1);				\
  }
  
#define DROP_TEMORARILY(uid) if (setresgid(-1, uid, getegid()) || setresuid(-1, uid, geteuid()) || getegid() != uid || geteuid() != uid ) \
    { perror("Could not drop privileges temporarily"); }
    
#define DROP_PERMANENTLY(uid) uid_t ruid, euid, suid; gid_t rgid, egid, sgid; if (setresgid(new_uid, new_uid, new_uid) || setresuid(new_uid, new_uid, new_uid) || getresgid(&rgid, &egid, &sgid) || getresuid(&ruid, &euid, &suid) || rgid != new_uid || egid != new_uid || sgid != new_uid || ruid != new_uid || euid != new_uid || suid != new_uid || getegid() != new_uid || geteuid() != new_uid ) { perror("Could not drop privileges permanently");

#define RESTORE_PRIVILEGES() uid_t ruid, euid, suid; gid_t rgid, egid, sgid if (getresgid(&rgid, &egid, &sgid) || getresuid(&ruid, &euid, &suid) || setresuid(-1, suid, -1) || setresgid(-1, sgid, -1) || geteuid() != suid || getegid() != sgid ) { perror("Could not restore privileges"); }

// Function declarations
const char * const to_string(long long int n, unsigned char base);
void copy_file(const char* source, const char* destination);
