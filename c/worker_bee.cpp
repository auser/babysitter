/** includes **/
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <sys/types.h>
#include "worker_bee.h"

/** Build the chroot at the path **/
bool WorkerBee::build_chroot(std::string &path, string_set &executables) {
  // Make the root path
  printf("Making directory at: %s\n", path.c_str());
  make_path(strdup(path.c_str()));
  
  // Build the root libraries
  for (string_set::iterator executable = executables.begin(); executable != executables.end(); ++executable) {
    printf("Looking at executables: %s\n", executable->c_str());
    string_set s_libs = *libs_for(*executable);
    for (string_set::iterator s = s_libs.begin(); s != s_libs.end(); ++s) {
      printf("- %s\n", s->c_str());
    }
  }
  return true;
}

string_set *WorkerBee::libs_for(const std::string &executable) {
  std::pair<string_set *, string_set *> *dyn_libs = linked_libraries(executable); 
  string_set *libs = new string_set();
  
  // iterate through
  string_set obj = *dyn_libs->first;
  // Go through the libs
  for (string_set::iterator ld = obj.begin(); ld != obj.end(); ++ld) {
    string_set paths = *dyn_libs->second;

    for (string_set::iterator pth = paths.begin(); pth != paths.end(); ++pth) {
      std::string full_path = *pth+'/'+*ld;
      if (fopen(full_path.c_str(), "rb") != NULL) {
        libs->insert(full_path);
      }
    }
  }
  
  return libs;
}

bool WorkerBee::matches_pattern(const std::string & matchee, const char * pattern, int flags) {
  regex_t xprsn;
  if (regcomp(&xprsn, pattern, flags|REG_EXTENDED|REG_NOSUB)) {
    fprintf(stderr, "Failed to compile regex\n");
    return(-1); // we are in the fork
  }

  if (0 == regexec(&xprsn, matchee.c_str(), 1, NULL, 0)) {
    regfree(&xprsn);
    return true;
  }

  regfree(&xprsn);
  return false;
}

bool WorkerBee::is_lib(const std::string &n) {
  return matches_pattern(n, "^lib(.*)+\\.so[.0-9]*$", 0);
}

std::pair<string_set *, string_set *> *WorkerBee::linked_libraries(const std::string executable) {
  /* open the offending executable  */
  int fl = open(executable.c_str(), O_RDONLY);
  if (-1 == fl) {
    fprintf(stderr, "Could not open %s\n", executable.c_str());
    return NULL;
  }
  
  /* Make sure we are working with a version of libelf */
  if (EV_NONE == elf_version(EV_CURRENT)) {
    fprintf(stderr, "ELF libs failed to initialize\n");
    return NULL;
  }

  // Start the elf interrogation
  Elf *elf = elf_begin(fl, ELF_C_READ, NULL);
  if (NULL == elf) {
    fprintf(stderr, "elf_begin failed because %s\n", elf_errmsg(-1));
    return NULL;
  }

  // Show deps
  GElf_Ehdr ehdr;
  if (!gelf_getehdr(elf, &ehdr)) {
    fprintf(stderr, "elf_getehdr failed from %s\n", elf_errmsg(-1));
    return NULL;
  }
  
  // scanning the headers
  string_set *libs = new string_set(), *paths = new string_set();
  // include the standard library paths
  paths->insert(""); // So we get full-path libraries included
  paths->insert("/lib");
  paths->insert("/usr/lib");
  paths->insert("/usr/local/lib");
  // include standard libraries to copy
  libs->insert("libdl.so.2");
  libs->insert("libm.so.2");
  libs->insert("libpthread.so.0");
  libs->insert("libattr.so.1");
  
  // Start scanning the header 
  Elf_Scn *scn = elf_nextscn(elf, NULL);
  GElf_Shdr shdr;
  while (scn) {
    if (NULL == gelf_getshdr(scn, &shdr)) {
      fprintf(stderr, "getshdr() failed from %s\n", elf_errmsg(-1));
      return NULL;
    }

    // get the name of the section (could optimize)
    char * nm = elf_strptr(elf, ehdr.e_shstrndx, shdr.sh_name);
    if (NULL == nm) {
      fprintf(stderr, "elf_strptr() failed from %s\n", elf_errmsg(-1));
      return NULL;
    }

    // look through the headers for the .dynstr and .interp headers
    if (strcmp(nm, ".bss")) {
      Elf_Data *data = NULL;
      size_t n = 0;
      // for each header, find the name if it matches the library name regex
      while (n < shdr.sh_size && (data = elf_getdata(scn, data)) ) {
        char *bfr = static_cast<char *>(data->d_buf);
        char *p = bfr + 1;
        while (p < bfr + data->d_size) {
          if (is_lib(p)) {
            libs->insert(p);
          }

          size_t lngth = strlen(p) + 1;
          n += lngth;
          p += lngth;
        }
      }
    }

    scn = elf_nextscn(elf, scn);
  }
 
  return new std::pair<string_set *, string_set*> (libs, paths);
}

int WorkerBee::cp_r(std::string &source, std::string &dest) {
  make_path(dirname(strdup(dest.c_str()))); 
  return cp(source, dest);
}
int WorkerBee::cp(std::string & source, std::string & destination) {
  struct stat stt;
  if (0 == stat(destination.c_str(), &stt)) {return -1;}

  FILE *src = fopen(source.c_str(), "rb");
  if (NULL == src) { return -1;}

  FILE *dstntn = fopen(destination.c_str(), "wb");
  if (NULL == dstntn) {
    fclose(src);
    return -1;
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
    return(-1);
  }
  return 0;
}
int WorkerBee::make_path(const std::string & path) {
  struct stat stt;
  if (0 == stat(path.c_str(), &stt) && S_ISDIR(stt.st_mode)) return -1;

  std::string::size_type lngth = path.length();
  for (std::string::size_type i = 1; i < lngth; ++i) {
    if (lngth - 1 == i || path[i] == '/') {
      std::string p = path.substr(0, i + 1);
      if (mkdir(p.c_str(), 0750) && EEXIST != errno && EISDIR != errno) {
        fprintf(stderr, "Could not create the directory %s", p.c_str());
        return(-1);
      }
    }
  }
  return 0;
}

std::string WorkerBee::find_binary(const std::string& file) {
  // assert(geteuid() != 0 && getegid() != 0); // We can't be root to run this.
  
  if (abs_path(file)) return file;
  
  std::string pth = DEFAULT_PATH;
  char *p2 = getenv("PATH");
  if (p2) {pth = p2;}
  
  std::string::size_type i = 0;
  std::string::size_type f = pth.find(":", i);
  do {
    std::string s = pth.substr(i, f - i) + "/" + file;
    
    if (0 == access(s.c_str(), X_OK)) {return s;}
    i = f + 1;
    f = pth.find(':', i);
  } while(std::string::npos != f);
  
  if (!abs_path(file)) {
    fprintf(stderr, "Could not find the executable %s in the $PATH\n", file.c_str());
    return NULL;
  }
  if (0 == access(file.c_str(), X_OK)) return file;
  
  fprintf(stderr, "Could not find the executable %s in the $PATH\n", file.c_str());
  return NULL;
}

bool WorkerBee::abs_path(const std::string & path) {
  return '/' == path[0] || ('.' == path[0] && '/' == path[1]);
}
