#include "fs.h"

// Die if we can't chdir to a new path.
void xchdir(const char *path)
{
	if (chdir(path)) fperror("chdir(%s)", path);
}

// Recursively compute the size of a directory
int dir_size_r(const char *fn)
{
  DIR *d;
  struct dirent *de;
  struct stat buf;
  int exists;
  int total_size;
  char *s;
  
  d = opendir(fn);
  if (d == NULL) {
    if ((exists = stat(fn, &buf)) < 0) return 0;
    return buf.st_size;
  }
 
  s = (char *) malloc(sizeof(char)*(strlen(fn)+258));
  total_size = 0;

  for (de = readdir(d); de != NULL; de = readdir(d)) {
    /* Look for fn/de->d_name */
    sprintf(s, "%s/%s", fn, de->d_name);
    exists = stat(s, &buf);
    if (exists < 0) {
      fprintf(stderr, "Couldn't stat %s\n", s);
    } else {
      total_size += buf.st_size;
    }
    if (S_ISDIR(buf.st_mode) && strcmp(de->d_name, ".") != 0 && 
        strcmp(de->d_name, "..") != 0) {
      total_size += dir_size_r(s);
    }
  }
  closedir(d);
  free(s);
  return total_size;
}

// Make the path and all parent directories
int mkdir_p(std::string dir, uid_t user, gid_t group, mode_t mode)
{
    if (mkdir(dir.c_str(), S_IRWXG | S_IRWXO | S_IRWXU) < 0) {
      if (errno == ENOENT) {
        size_t slash = dir.rfind('/');
        if (slash != std::string::npos) {
   	      std::string prefix = dir.substr(0, slash);
   	      
   	      if (mkdir_p(prefix, user, group, mode)) {
            fprintf(stderr, "Could not create the directory prefix: %s: %s\n", prefix.c_str(), ::strerror(errno));
            return -1;
   	      }
   	       
          if (mkdir(dir.c_str(), S_IRWXG | S_IRWXO | S_IRWXU)) {
            fprintf(stderr, "Could not create the directory: %s: %s\n", dir.c_str(), ::strerror(errno));
            return -1;
          }
        }
      }
    }
    // if (chown(dir.c_str(), user, group)) {
    //   fprintf(stderr, "Could not change ownership of %s to %o:%o: %s\n", dir.c_str(), user, group, ::strerror(errno));
    //   return -1;
    // }
  return 0;
}

int mkdir_p(std::string dir)
{
  if (mkdir(dir.c_str(), S_IRWXG | S_IRWXO | S_IRWXU) < 0) {
    if (errno == ENOENT) {
      size_t slash = dir.rfind('/');
      if (slash != std::string::npos) {
 	      std::string prefix = dir.substr(0, slash);
 	      
 	      if (mkdir_p(prefix)) {
          fprintf(stderr, "Could not create the directory prefix: %s: %s\n", prefix.c_str(), ::strerror(errno));
          return -1;
 	      }
 	       
        if (mkdir(dir.c_str(), S_IRWXG | S_IRWXO | S_IRWXU)) {
          fprintf(stderr, "Could not create the directory: %s: %s\n", dir.c_str(), ::strerror(errno));
          return -1;
        }
      }
    }
  }
  return 0;
}

// Recursively remove a directory
int rmdir_p(std::string directory)
{
  assert( (directory != "") );
  
  DIR           *d;
  struct dirent *dir;
  
  d = opendir( directory.c_str() );
  if( d == NULL ) {
    return 1;
  }
  while( ( dir = readdir( d ) ) ) {
    if( strcmp( dir->d_name, "." ) == 0 || strcmp( dir->d_name, ".." ) == 0 ) continue;

    // If this is a directory
    if( dir->d_type == DT_DIR ) {
      std::string next_dir (directory + "/" + dir->d_name);
      rmdir_p(next_dir);
    } else {
      std::string file = (directory + "/" + dir->d_name);
      assert( file != "" );
      unlink(file.c_str());
    }
  }
  
  closedir( d );
  
  if (rmdir(directory.c_str()) < 0) {
    fprintf(stderr, "Couldn't delete directory: %s\n", directory.c_str());
    return -1;
  }
  return 0;
}