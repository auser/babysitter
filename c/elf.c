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

int main(int argc, char *argv[])
{

int fd; 		// File Descriptor
char* base_ptr;		// ptr to our object in memory
char* file = argv[1];	// filename
struct stat elf_stats;	// fstat struct

  /* open the offending file  */
  int fl = open(file, O_RDONLY);
  if (-1 == fl) {
    fprintf(stderr, "Could not open %s\n", file);
    return(-1);
  }

  if (EV_NONE == elf_version(EV_CURRENT)) {
    fprintf(stderr, "ELF libs failed to initialize\n");
    return(-1);
  }

  // Start the elf interrogation
  Elf *elf = elf_begin(fl, ELF_C_READ, NULL);
  if (NULL == elf) {
    fprintf(stderr, "elf_begin failed because %s\n", elf_errmsg(-1));
    return(-1);
  }

/* Check libelf version first */
if (elf_version(EV_CURRENT) == EV_NONE) {
  printf("WARNING Elf Library is out of date!\n");
}

 
// Show deps
  GElf_Ehdr ehdr;
  if (!gelf_getehdr(elf, &ehdr)) {
    fprintf(stderr, "elf_getehdr failed from %s\n", elf_errmsg(-1));
    return NULL;
  }
  
  // scanning the headers
  string_set *lbrrs = new string_set(), *pths = new string_set();
  
  Elf_Scn *scn = elf_nextscn(elf, NULL);
  GElf_Shdr shdr;
  while (scn) {
    if (NULL == gelf_getshdr(scn, &shdr)) {
      fprintf(stderr, "getshdr() failed from %s\n", elf_errmsg(-1));
      return NULL;
    }

    char * nm = elf_strptr(elf, ehdr.e_shstrndx, shdr.sh_name);
    if (NULL == nm) {
      fprintf(stderr, "elf_strptr() failed from %s\n", elf_errmsg(-1));
      return NULL;
    }

    //if (SHT_DYNSYM == shdr.sh_type) break;
  if (!strcmp(nm, ".interp") || !strcmp(nm, ".dynstr")) {
    Elf_Data *data = NULL;
    size_t n = 0;
    while (n < shdr.sh_size && (data = elf_getdata(scn, data)) ) {
      char *bfr = static_cast<char *>(data->d_buf);
      char *p = bfr + 1;
      while (p < bfr + data->d_size) {
        if (is_lib(p)) {
          printf("p: '%s'\n", p);
          lbrrs->insert(p);
        } else if (p[0] == '/') {
          printf("Adding path: %s\n", p);
          pths->insert(p);
        }

      size_t lngth = strlen(p) + 1;
      n += lngth;
      p += lngth;
    }
  }
}

  scn = elf_nextscn(elf, scn);
}
printf("Ready\n");
}
