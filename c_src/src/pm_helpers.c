#include "pm_helpers.h"

int pm_abs_path(const char *path)
{
  if ((path[0] == '/') || ((path[0] == '.') && (path[1] == '/')))
    return 0;
  else
    return -1;
}

const char *find_binary(const char *file)
{
  char buf[BUFFER_SZ];
  const char *p, *path;
  int len = 0, lp;
  
  if (file[0] == '\0') {
    errno = ENOENT;
    return NULL;
  }
  if (!strcmp(file,"")) return "";
  if (!pm_abs_path(file)) return file;
  
  // Get the path
  if (!(path = getenv("PATH"))) path = _PATH_DEFPATH;
	
  len = strlen(file);
  do {
    /* Find the end of this path element. */
		for (p = path; *path != 0 && *path != ':'; path++)
			continue;
		
		// If the path has a '.' at it, then it's local
		if (p == path) {
			p = ".";
			lp = 1;
		} else lp = path - p;
		    
    (void*)memset(buf, '\0', BUFFER_SZ);
    memcpy(buf, p, lp);
		buf[lp] = '/';
		memcpy(buf + lp + 1, file, len);
		buf[lp + len + 1] = '\0';
		
    if (0 == access(buf, X_OK)) {return strdup(buf);}
    
  } while(*path++ == ':');
  
  return "\0";
}