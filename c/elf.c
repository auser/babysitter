#include <stdio.h>
#include <string.h>
#include <error.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gelf.h>
#include <set>
#include <regex.h>

/** Worker bee **/
#include "worker_bee.h"

#define ERR -1

typedef std::set<std::string> string_set;

Elf32_Ehdr *elf_header;		/* ELF header */
Elf *elf;                       /* Our Elf pointer for libelf */
Elf_Scn *scn;                   /* Section Descriptor */
Elf_Data *edata;                /* Data Descriptor */
GElf_Sym sym;			/* Symbol */
GElf_Shdr shdr;                 /* Section Header */

bool matches_pattern(const std::string & matchee, const char * pattern, int flags) {
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

bool is_lib(const std::string &n) {
  return matches_pattern(n, "^lib(.*)+\\.so[.0-9]*$", 0);
}

std::pair<string_set *, string_set *> *linked_libraries(char *file) {
  int fd;                      // File Descriptor
  string_set *already_copied;  // Already copied libs

  /* open the offending file  */
  int fl = open(file, O_RDONLY);
  if (-1 == fl) {
    fprintf(stderr, "Could not open %s\n", file);
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

int main(int argc, char **argv) {
  /*
  std::pair<string_set *, string_set *> *dyn_libs = linked_libraries(argv[1]); 
  
  // iterate through
  string_set obj = *dyn_libs->first;
  // Go through the libs
  for (string_set::iterator ld = obj.begin(); ld != obj.end(); ++ld) {
    string_set paths = *dyn_libs->second;
    bool found = false;

    for (string_set::iterator pth = paths.begin(); pth != paths.end(); ++pth) {
      std::string full_path = *pth+'/'+*ld;
      if (fopen(full_path.c_str(), "rb") != NULL) {
        printf("\t%s\n", full_path.c_str());
      }
    }
  }
  */
  WorkerBee b(argv[1]);
  b.libs();
  return 0;
}
