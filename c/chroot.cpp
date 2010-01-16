/**
 * Change the current user into a non-root user if we are running as a root user
 **/
void change_user_to_non_root_user() {
  if (getuid() == 0) {
    superuser = true;
    if (userid == 0) {
      fprintf(stderr, "When running as root, \"-user User\" option must be provided!");
      //exit(4); UNCOMMENT THIS!
    }
     	if (prctl(PR_SET_KEEPCAPS, 1) < 0) {
  		perror("Failed to call prctl to keep capabilities");
    	exit(5);
  	}
      if (setresuid(userid, userid, userid) < 0) {
          perror("Failed to set userid");
          exit(6);
      }

      struct passwd* pw;
      if (debug && (pw = getpwuid(geteuid())) != NULL)
          fprintf(stderr, "exec: running as: %s (uid=%d)\r\n", pw->pw_name, getuid());

      cap_t cur;
      if ((cur = cap_from_text("cap_setuid=eip cap_kill=eip cap_sys_nice=eip")) == 0) {
          perror("Failed to convert cap_setuid & cap_sys_nice from text");
          exit(7);
      }
      if (cap_set_proc(cur) < 0) {
          perror("Failed to set cap_setuid & cap_sys_nice");
          exit(8);
      }
      cap_free(cur);

      if (debug && (cur = cap_get_proc()) != NULL) {
          fprintf(stderr, "exec: current capabilities: %s\r\n",  cap_to_text(cur, NULL));
          cap_free(cur);
      }
  }
}

