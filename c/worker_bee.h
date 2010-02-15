#ifndef WORKER_BEE_H
#define WORKER_BEE_H 

/**
 * Worker bee
 * This class is responsible for finding the dependencies
 * required by a binary and building the full path
 * for the dependencies
 **/
/** includes **/
#include <stdio.h>
#include <error.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gelf.h>
#include <pwd.h>
#include <set>
#include <regex.h>

#include "honeycomb_config.h"

/** defines **/
#define ERR -1
#define DEFAULT_PATH "/bin:/usr/bin:/usr/local/bin:/sbin;"

/** types **/
// Beefiles are for chroots
class BeeFile {
private:
  std::string m_file_path;
  // If there is a symlink, use this to point to the original file
  std::string m_sym_origin;
  mode_t m_file_mode;
  bool m_is_link;

public:
  BeeFile() : m_is_link(false) {}
  ~BeeFile() {}

  std::string file_path() {return m_file_path;}
  std::string sym_origin() {return m_sym_origin;}
  mode_t      file_mode() {return m_file_mode;}
  bool        is_link() {return m_is_link;}
  // Really REALLY simple setters
  void        set_file_path(const char *nfp) {m_file_path = nfp;}
  void        set_sym_origin(const char *nfp) {m_sym_origin = nfp;}
  void        set_file_mode(mode_t mode) {m_file_mode = mode;}
  void        set_is_link(bool b) {m_is_link = b;}
};
// Simply so we can filter through the bee files using an iterator
class BeeCompare {
public:
  bool operator()(BeeFile b1, BeeFile b2) {return (strcmp(b1.file_path().c_str(), b2.file_path().c_str()) > 0);}
};

typedef std::set<std::string> string_set;
typedef std::set<BeeFile, BeeCompare> bee_files_set;

class WorkerBee {
private:
  Elf32_Ehdr *elf_header;	  	// Elf header object reference
  Elf *elf;                   // Elf reference
  Elf_Scn *scn;               // Seciton Descriptor
  Elf_Data *edata;            // Data description
  GElf_Sym sym;               // Symbols
  GElf_Shdr shdr;             // Section header
  
public:
  WorkerBee() {}
  ~WorkerBee() {}

  bee_files_set *libs_for(const std::string &str);
  bool build_base_dir(const std::string &path, uid_t user, gid_t group);
  int  build_chroot(const std::string &path, uid_t user, gid_t group, string_set &executables, string_set &ef, string_set &extra_dirs);
  int  secure_chroot(std::string m_cd);
  
  int cp_rf(std::string directory, std::string path);
  
// Functions
private:
  bool matches_pattern(const std::string & matchee, const char * pattern, int flags);
  bool is_lib(const std::string &n);
  std::pair<string_set *, string_set *> *linked_libraries(const std::string str);
  /** Utilities **/
  int make_path(const std::string & path);
  int mkdir_r(const std::string & path);
  int cp_f(const std::string &source, const std::string &dest);
  int cp(const std::string & source, const std::string & destination);
  int copy_binary_file(std::string path, std::string res_bin, uid_t user, gid_t group);
  std::string find_binary(const std::string& file);
  bool abs_path(const std::string & path);
};

#endif