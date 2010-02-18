/** includes **/
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <dirent.h> 

#include "worker_bee.h"
#include "honeycomb.h"

// Build a base directory
bool WorkerBee::build_base_dir(const std::string &path, uid_t user, gid_t group, string_set &extra_dirs) {
  std::string full_path;
  make_path(strdup(path.c_str()));
  
  // Make sure the base path exists
  if (chown(path.c_str(), user, group) != 0) {
	  fprintf(stderr, "[build_base_dir] Could not change owner of '%s' to %i\n", path.c_str(), user);
    return false;
	}
	
	string_set base_dirs;
  base_dirs.insert("bin");
  base_dirs.insert("usr");
  base_dirs.insert("var");
  base_dirs.insert("lib");
  base_dirs.insert("home");
  base_dirs.insert("etc");
  
  // Add the extra directories requested
  for (string_set::iterator dir = extra_dirs.begin(); dir != extra_dirs.end(); ++dir) base_dirs.insert(dir->c_str());
  
  for (string_set::iterator dir = base_dirs.begin(); dir != base_dirs.end(); ++dir) {
    if ((*dir->c_str()) == FS_SLASH) full_path = path + *dir; else full_path = path + FS_SLASH + *dir;
    
    // Make the paths
    make_path(full_path.c_str());
  }
  
  return true;
}

/** Build the chroot at the path **/
int WorkerBee::build_chroot(const std::string &path, 
                            uid_t user, 
                            gid_t group, 
                            string_set &executables, 
                            string_set &extra_files,
                            string_set &extra_dirs) {
  // Make the root path
  make_path(strdup(path.c_str()));
  string_set already_copied;
  std::string full_path;
  
  // The base directories  
  string_set base_dirs;
  base_dirs.insert("bin");
  base_dirs.insert("usr");
  base_dirs.insert("var");
  base_dirs.insert("lib");
  base_dirs.insert("home");
  base_dirs.insert("etc");
  
  // Base executables
  // TODO: Move this into a base conf. path
  executables.insert("which");
  executables.insert("cat");
  executables.insert("ls");
  
  // Add the extra directories requested
  for (string_set::iterator dir = extra_dirs.begin(); dir != extra_dirs.end(); ++dir) base_dirs.insert(dir->c_str());
  
  for (string_set::iterator dir = base_dirs.begin(); dir != base_dirs.end(); ++dir) {
    if ((*dir->c_str()) == FS_SLASH) full_path = path + *dir; else full_path = path + FS_SLASH + *dir;
    
    // Make the paths
    make_path(full_path.c_str());
  }
  
  // Now recursively copy the extra_dirs
  for (string_set::iterator dir = extra_dirs.begin(); dir != extra_dirs.end(); ++dir) {
    std::string s_dir (path + "/" + *dir);
    mkdir_r(strdup(s_dir.c_str()));
    cp_rf(*dir, s_dir.c_str());
  }
  
  for (string_set::iterator file = extra_files.begin(); file != extra_files.end(); ++file) {
    if ((*file).c_str()[0] == FS_SLASH) full_path = path + (*file).c_str(); else full_path = path + FS_SLASH + (*file).c_str();
    
    make_path(dirname(strdup(full_path.c_str())));
    cp_f(*file, full_path);
  }
  
  // Build the root libraries
  for (string_set::iterator executable = executables.begin(); executable != executables.end(); executable++) {
    // If we are pointed at an absolute path to a binary
    // then find the linked libraries of the executable
    // If it's not found, then find it, then look up the libraries
    std::string res_bin;
    if (abs_path(*executable))
      res_bin = *executable;
    else {
      res_bin = find_binary(*executable);
    }
    
    if (res_bin != "") {
      // The libraries for the resolved binary 
      bee_files_set *s_libs = libs_for(res_bin);
      
      // collect the libraries and copy them to the full path of the chroot
      for (bee_files_set::iterator bf = s_libs->begin(); bf != s_libs->end(); ++bf) {
        BeeFile bee = *bf;
        std::string s (bee.file_path());
      
        // Don't copy if the file is already copied
        if (already_copied.count(s)) {
        } else {
          // If it is a symlink, then make a symlink, otherwise copy the file
          // If the library starts with a FS_SLASH then don't add one, otherwise, do
          // i.e. if libs/hi.so.1 turns into full_path/libs/hi.so.1 otherwise libs.so.1 turns into full_path/libs.so.1
          if (s.c_str()[0] == FS_SLASH) full_path = path + s.c_str(); else full_path = path + FS_SLASH + s.c_str();
        
          if (bee.is_link()) {
            // make symlink
            make_path(dirname(strdup(full_path.c_str()))); 
            if (symlink(bee.sym_origin().c_str(), full_path.c_str())) {
              fprintf(stderr, "Could not make a symlink: %s to %s because %s\n", bee.sym_origin().c_str(), full_path.c_str(), strerror(errno));
            }
          } else {
            // Copy the file (recursively)
            cp_f(s, full_path);
          }
          // Change the permissions to match the original file
          mode_t mode = bee.file_mode();
  			
    			if (chown(full_path.c_str(), user, group) != 0) {
#ifdef DEBUG
           printf("[build_chroot] Could not change owner of '%s' to %d: %s\n", full_path.c_str(), (int)user, ::strerror(errno));
#endif
    			}
          
          if (chmod(full_path.c_str(), mode) != 0) {
#ifdef DEBUG
            printf("Could not change permissions to '%s' %o\n", full_path.c_str(), (int)mode);
#endif
          }
          // Add it to the already_copied set and move on
          already_copied.insert(s);
        }
      }
    
      // Copy the executables and make them executable
      copy_binary_file(path, res_bin, user, group);
    }		
  }
  return 0;
}

/**
* Recursively copy a direcotry
**/
int WorkerBee::cp_rf(std::string directory, std::string path) {
  DIR           *d;
  struct dirent *dir;
  
  d = opendir( directory.c_str() );
  if( d == NULL ) {
    return 1;
  }
  while( ( dir = readdir( d ) ) ) {
    if( strcmp( dir->d_name, "." ) == 0 || strcmp( dir->d_name, ".." ) == 0 ) continue;

    if( dir->d_type == DT_DIR ) {
      std::string next_dir (directory + "/" + dir->d_name);
      mkdir_r(next_dir);
      cp_rf( next_dir, path + "/" + dir->d_name );
    } else {
      std::string file = (directory + "/" + dir->d_name);
      cp_f( file, path + "/" + dir->d_name );
    }
  }
  closedir( d );
  return 0;
}


/**
* Copy a binary file into the path
* Copy their symlinks if necessary
**/
int WorkerBee::copy_binary_file(std::string path, std::string res_bin, uid_t user, gid_t group) {
  // Copy the executables and make them executable
  std::string bin_path = path + FS_SLASH + res_bin;
  struct stat bin_stat;
  
  if (lstat(res_bin.c_str(), &bin_stat) < 0) {
    fprintf(stderr, "[lstat] Error: %s: %s\n", res_bin.c_str(), strerror(errno));
  }
  // Are we looking at a symlink
  if (S_ISLNK(bin_stat.st_mode)) {
    char link_buf[1024];
    memset(link_buf, 0, 1024);
    if (!readlink(res_bin.c_str(), link_buf, 1024)) {
      fprintf(stderr, "[readlink] Error: %s: %s\n", res_bin.c_str(), strerror(errno));
    }
    
    std::string link_dir (dirname(strdup(res_bin.c_str())));
    std::string link_path, link_origin;
    // If the linked library has no / at the beginning of the string, then it clearly is in the same directory
    if (link_buf[0] != FS_SLASH) {
      link_origin = link_dir + "/" + link_buf;
    } else {
      link_origin = link_buf;
    }
    // std::string link_path (link_buf);
    link_path = (path+"/"+link_dir+"/"+link_buf);
    
    cp_f(link_origin.c_str(), link_path.c_str());
    if (chown(link_path.c_str(), user, group) != 0) {
  	  fprintf(stderr, "[copy_binary_file] Could not change owner of '%s' to %i\n", link_path.c_str(), user);
  	}

    if (chmod(link_path.c_str(), S_IREAD|S_IEXEC|S_IXGRP|S_IRGRP|S_IWRITE)) {
      fprintf(stderr, "Could not change permissions to '%s' make it executable\n", link_path.c_str());
    }
    
  }
  
  // Copy the original into the path
  cp_f(res_bin.c_str(), bin_path.c_str());
  if (chown(bin_path.c_str(), user, group) != 0) {
	  fprintf(stderr, "[copy_binary_file original] Could not change owner of '%s' to %i\n", bin_path.c_str(), user);
	}

  if (chmod(bin_path.c_str(), S_IREAD|S_IEXEC|S_IXGRP|S_IRGRP|S_IWRITE)) {
    fprintf(stderr, "Could not change permissions to '%s' make it executable\n", bin_path.c_str());
  }
  
  return 0;
}

int WorkerBee::secure_chroot(std::string m_cd) {
  // Secure the chroot first
  unsigned int fd, fd_max;
  struct stat buf; struct rlimit lim;
  if (! getrlimit(RLIMIT_NOFILE, &lim) && (fd_max < lim.rlim_max)) fd_max = lim.rlim_max;

  // compute the number of file descriptors that can be open
#ifdef OPEN_MAX
  fd_max = OPEN_MAX;
#elif defined(NOFILE)
  fd_max = NOFILE;
#else
  fd_max = getdtablesize();
#endif

  // close all file descriptors except stdin,stdout,stderr
  // because they are security issues
  DEBUG_MSG("Securing the chroot environment at '%s'\n", m_cd.c_str());
  for (fd=2;fd < fd_max; fd++) {
    if ( !fstat(fd, &buf) && S_ISDIR(buf.st_mode))
      if (close(fd)) {
        fprintf(stderr, "Could not close insecure directory/file: %i before chrooting\n", fd);
        return(-1);
      }
  }
  return 0;
}

bee_files_set *WorkerBee::libs_for(const std::string &executable) {
  std::pair<string_set *, string_set *> *dyn_libs;
  
  // If we are pointed at an absolute path to a binary
  // then find the linked libraries of the executable
  // If it's not found, then find it, then look up the libraries
  if (abs_path(executable))
    dyn_libs = linked_libraries(executable);
  else {
    std::string bin = find_binary(executable);
    dyn_libs = linked_libraries(bin);
  }
  //string_set *libs = new string_set();
  bee_files_set *libs = new bee_files_set();  

  // iterate through
  string_set obj = *dyn_libs->first;
  char link_buf[1024];
  // Go through the libs
  for (string_set::iterator ld = obj.begin(); ld != obj.end(); ++ld) {
    string_set paths = *dyn_libs->second;
    
    // Go through each of the paths
    for (string_set::iterator pth = paths.begin(); pth != paths.end(); ++pth) {
      std::string path (*pth);
      std::string full_path = *pth+FS_SLASH+*ld;
      if (fopen(full_path.c_str(), "rb") != NULL) {
        
        // Create a bee_file object
        BeeFile bf;
        struct stat lib_stat;
        bf.set_file_path(full_path.c_str());
        
        // Make sure the file can be "stat'd"
        if (lstat(full_path.c_str(), &lib_stat) < 0) {
          fprintf(stderr, "[lstat] Error: %s: %s\n", full_path.c_str(), strerror(errno));
        }
        bf.set_file_mode(lib_stat.st_mode);
			  // Are we looking at a symlink
        // if ((lib_stat.st_mode & S_IFMT) == S_IFLNK) {
        if (S_ISLNK(lib_stat.st_mode)) {

          memset(link_buf, 0, 1024);
          if (!readlink(full_path.c_str(), link_buf, 1024)) {
            fprintf(stderr, "[readlink] Error: %s: %s\n", full_path.c_str(), strerror(errno));
          }
          
          /** If we are looking at a symlink, then create a new BeeFile object and 
           * insert it into the library path, noting that the other is a symlink
          **/ 
          std::string link_dir (dirname(strdup(full_path.c_str())));
          // std::string link_path (link_buf);
          std::string link_path;
          if (path == "") 
            link_path = link_dir + "/" + link_buf; 
          else 
            link_path = (path+"/"+link_buf);
          
          BeeFile lbf;
          struct stat link_stat;
          // Set the data on the BeeFile object
          lbf.set_file_path(link_path.c_str());
          if (lstat(link_path.c_str(), &link_stat)) {
            fprintf(stderr, "[lstat] Could not read stats for %s\n", link_path.c_str());
          }
          lbf.set_file_mode(link_stat.st_mode);
          
          // full_path.c_str()
          bf.set_file_path(full_path.c_str()); // This is redundant, but just for clarity

          bf.set_sym_origin(link_buf);
          bf.set_is_link(true);
          bf.set_file_mode(100755);
          
          libs->insert(lbf);
        } else {
          bf.set_is_link(false);
        }
        
        libs->insert(bf);
        break; // We found it! Move on, yo
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
  // scanning the headers
  string_set *libs = new string_set(), *paths = new string_set();
  
  /* open the offending executable  */
  int fl = open(executable.c_str(), O_RDONLY);
  if (-1 == fl) {
    fprintf(stderr, "Could not open %s\n", executable.c_str());
    return new std::pair<string_set *, string_set*> (libs, paths);
  }
  
  int sh_length = 2;
  char *buf[sh_length];
  memset(buf, 0, sh_length);
  
  paths->insert(""); // So we get full-path libraries included
  // If we are reading a script...
  if ((read(fl, buf, sh_length)) && (strncmp((const char*)buf, "#!", sh_length) == 0)) {
    
    // Make sure we include the interpreter
    const int max_interpreter_len = 80;
    char line[max_interpreter_len];
    memset(buf, 0, max_interpreter_len);
    FILE *fd = fopen(executable.c_str(), "r");
    
    if (fgets(line, max_interpreter_len, fd)) {
      int len = (int)strlen(line);
      
      // Strip off the #!
      for (int j = 0; j < len; ++j) {
        line[j] = line[j+2];
      }
      line[len-3] = 0; // skip the newline at the end
      
      close(fl); fclose(fd);
#ifdef DEBUG
      printf("found an interpreter: %s\n", line);
#endif

      // Chomp the beginning of the line
      for(int i = 0; i < len; ++i) {
        if ( isspace(line[i]) ) {        
          for (int j = 0; j < len; ++j) {
            line[j] = line[j+1];
          }
          line[len--] = 0;
        } else {
          break;
        }
      }
      // Chomp the end of the line
      while ((len>=0) && (isspace(line[len])) ) {
        line[len] = 0;
        len--;
      }
    
      std::pair<string_set *, string_set*> *linked_libs = linked_libraries(line);
      // We need to include the interpreter!
      (linked_libs->second)->insert(line);
      return linked_libs;
    } else {
      libs->insert(executable.c_str());
      close(fl); fclose(fd);
      return new std::pair<string_set *, string_set*> (libs, paths);
    }
    close(fl); fclose(fd);
  } else {
    /* Make sure we are working with a version of libelf */
    if (EV_NONE == elf_version(EV_CURRENT)) {
      fprintf(stderr, "ELF libs failed to initialize\n");
      return NULL;
    }

    // Start the elf interrogation
    Elf *elf = elf_begin(fl, ELF_C_READ, NULL);
    if (NULL == elf) {
      fprintf(stderr, "elf_begin failed because %s\n", elf_errmsg(-1));
      // return NULL;
    }

    // Show deps
    GElf_Ehdr ehdr;
    if (!gelf_getehdr(elf, &ehdr)) {
      fprintf(stderr, "elf_getehdr failed from %s\n", elf_errmsg(-1));
      // return NULL;
    }
  
    // include the standard library paths
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
    close(fl);
    return new std::pair<string_set *, string_set*> (libs, paths);
  }
}

/**
* Look into the /proc/[pid]/maps for the libraries required at runtime
**/
string_set* WorkerBee::libs_for_running_process(pid_t pid) {
  string_set *libs = new string_set();
  
  char fname[PATH_MAX];
  FILE *f;

  sprintf(fname, "/proc/%ld/maps", (long)pid);
  f = fopen(fname, "r");

  if(!f) {
    perror("libs_for_running_process:");
    fprintf(stderr, "%s: %s\n", fname, ::strerror(errno));
    return libs;
  }

  while(!feof(f)) {
	  char buf[PATH_MAX+100], perm[5], dev[6], mapname[PATH_MAX];
	  unsigned long begin, end, inode, foo;

	  if(fgets(buf, sizeof(buf), f) == 0) break;
	  mapname[0] = '\0';
	  sscanf(buf, "%lx-%lx %4s %lx %5s %ld %s", &begin, &end, perm, &foo, dev, &inode, mapname);	  
    libs->insert(mapname);
  }
  fclose(f);
  
  return libs;
}

int WorkerBee::cp_f(const std::string &source, const std::string &dest) {
  make_path(dirname(strdup(dest.c_str()))); 
  return cp(source, dest);
}
int WorkerBee::cp(const std::string & source, const std::string & destination) {
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

/**
* This will ensure that the directory you specify will exist
**/
int WorkerBee::make_path(const std::string & path) {
  struct stat stt;
  if (0 == stat(path.c_str(), &stt) && S_ISDIR(stt.st_mode)) return -1;
  
  std::string::size_type lngth = path.length();
  for (std::string::size_type i = 1; i < lngth; ++i) {
    if (lngth - 1 == i || path[i] == FS_SLASH) {
      std::string p = path.substr(0, i + 1);
      if (mkdir(p.c_str(), 0750) && EEXIST != errno && EISDIR != errno) {
#ifdef DEBUG
        fprintf(stderr, "Could not create the directory %s\n", p.c_str());
#endif
        return(-1);
      }
    }
  }
  return 0;
}

/**
* This will ensure that the directory you specify will exist
**/
int WorkerBee::mkdir_r(const std::string & path) {
  struct stat stt;
  if (0 == stat(path.c_str(), &stt) && S_ISDIR(stt.st_mode)) return -1;
  
  int pos = 0;
  const char *dir_buf = path.c_str();
  int len = strlen(dir_buf);
  char dir_path[BUF_SIZE];
  memset(dir_path, 0, BUF_SIZE);
  
  while (pos < len) {
    if ((dir_buf[pos] == FS_SLASH) && strlen(dir_buf) > 1) {
      // Make the dir
      printf("mkdir(%s)\n", dir_path);
      if (mkdir(dir_path, 0750) && EEXIST != errno && EISDIR != errno) {
#ifdef DEBUG
        fprintf(stderr, "Could not create the directory %s\n", dir_path);
#endif
      }
      dir_path[pos] = FS_SLASH;
    } else {
      dir_path[pos] = dir_buf[pos];
    }
    pos++;
  }
  return 0;
}

std::string WorkerBee::find_binary(const std::string& file) {
  // assert(geteuid() != 0 && getegid() != 0); // We can't be root to run this.
  
  if (file == "") return "";
  
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
  return FS_SLASH == path[0] || ('.' == path[0] && FS_SLASH == path[1]);
}
