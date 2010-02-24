#include <string.h>
#include <unistd.h>
#include <stdio.h>

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
