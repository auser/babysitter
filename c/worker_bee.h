/**
 * Worker bee
 * This class is responsible for finding the dependencies
 * required by a binary and building the full path
 * for the dependencies
 **/
/** includes **/
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

/** defines **/
#define ERR -1

/** types **/
typedef std::set<std::string> string_set;

class WorkerBee {
private:
  Elf32_Ehdr *elf_header;	  	// Elf header object reference
  Elf *elf;                   // Elf reference
  Elf_Scn *scn;               // Seciton Descriptor
  Elf_Data *edata;            // Data description
  GElf_Sym sym;               // Symbols
  GElf_Shdr shdr;             // Section header
  string_set *full_path_libs; // Full path for the libs
  std::string m_executable;
  
public:
  WorkerBee(std::string executable) : m_executable(executable) {}
  ~WorkerBee() {}

  string_set *libs();
  bool build_chroot(char *path);

// Functions
private:
  bool matches_pattern(const std::string & matchee, const char * pattern, int flags);
  bool is_lib(const std::string &n);
  std::pair<string_set *, string_set *> *linked_libraries();

};
