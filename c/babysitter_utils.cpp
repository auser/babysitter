#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

#include <string>

#ifndef BUF_SIZE
#define BUF_SIZE 1024
#endif

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
  if ( (sha_val_ret = (char *) malloc(sizeof(char *) * strlen(sha_val))) == NULL ) {
    fprintf(stderr, "Could not allocate a new char. Out of memory\n");
    exit(-1);
  }
  memcpy(sha_val_ret, sha_val, strlen(sha_val));
  
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