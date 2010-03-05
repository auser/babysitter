#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include <string>

#ifndef BUF_SIZE
#define BUF_SIZE 1024
#endif

int babysitter_system_error(int err, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "[ERROR] "); vfprintf(stderr, fmt, ap); va_end(ap);
  return err;
}

// Parse sha
// Pass the root directory of the git repos
const char *parse_sha_from_git_directory(std::string root_directory)
{
  std::string git_dir = root_directory + "/.git";
  std::string head_loc = git_dir + "/HEAD";
  
  FILE *fd = fopen(head_loc.c_str(), "r");
	if (!fd) {
    fprintf(stderr, "Could not open the file: '%s'\nCheck the permissions and try again\n", head_loc.c_str());
		return NULL;
	}
	
  char file_loc[BUF_SIZE];
  memset(file_loc, 0, BUF_SIZE); // Clear it out
  fscanf(fd, "ref: %s", file_loc);
  // memset(file_loc, 0, BUF_SIZE); // Clear it out
  // fgets(file_loc, BUF_SIZE, fd);
  fclose(fd);
  
  std::string sha_loc = git_dir + "/" + file_loc;
  fd = fopen(sha_loc.c_str(), "r");
	if (!fd) {
    fprintf(stderr, "Could not open the file: '%s'\nCheck the permissions and try again\n", sha_loc.c_str());
		return NULL;
	}
	
	char sha_val[BUF_SIZE];
  memset(sha_val, 0, BUF_SIZE); // Clear it out
  fgets(sha_val, BUF_SIZE, fd);
  fclose(fd);
  
  if (sha_val[strlen(sha_val) - 1] == '\n') sha_val[strlen(sha_val) - 1] = 0;
	
  char *sha_val_ret;
  if ( (sha_val_ret = (char *) malloc(sizeof(char) * strlen(sha_val))) == NULL ) {
    fprintf(stderr, "Could not allocate a new char. Out of memory\n");
    exit(-1);
  }
  memset(sha_val_ret, 0, strlen(sha_val)); // Clear it out
  memcpy(sha_val_ret, sha_val, strlen(sha_val));
  *(sha_val_ret + strlen(sha_val)) = '\0';
  
  return sha_val_ret;
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


#define SKIP(p) while (*p && isspace (*p)) p++
#define WANT(p) *p && !isspace (*p)

/* Count the number of arguments. */

static int count_args (const char * input)
{
    const char * p;
    int argc = 0;
    p = input;
    while (*p) {
        SKIP (p);
        if (WANT (p)) {
            argc++;
            while (WANT (p))
                p++;
        }
    }
    return argc;
}

/* Copy each non-whitespace argument into its own allocated space. */

static int copy_args (const char * input, int argc, char ** argv)
{
    int i = 0;
    const char *p;

    p = input;
    while (*p) {
        SKIP (p);
        if (WANT (p)) {
            const char * end = p;
            char * copy;
            while (WANT (end))
                end++;
            copy = argv[i] = (char *)malloc (end - p + 1);
            if (! argv[i])
                return -1;
            while (WANT (p))
                *copy++ = *p++;
            *copy = 0;
            i++;
        }
    }
    if (i != argc) 
        return -1;
    return 0;
}

#undef SKIP
#undef WANT

int argify (const char * input, int * argc_ptr, char *** argv_ptr)
{
    int argc;
    char ** argv;

    argc = count_args (input);
    if (argc == 0)
        return -1;
    argv = (char **)malloc (sizeof (char *) * argc);
    if (! argv)
        return -1;
    if (copy_args (input, argc, argv) == -1)
        return -1;
    *argc_ptr = argc;
    *argv_ptr = argv;
    return 0;
}