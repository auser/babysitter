int babysit(const char *cmd, char* const* env);

struct BabySitterInfo {
  std::string    cmd;         // Command to execute
  rlim_t         r_limits;    // Limits for chroot
  char *         env_vars;  // Environment variables
  std::string    image_path;  // Image to mount
  
  BabySitterInfo() {}
  BabySitterInfo(const char* _cmd, rlim_t _limits, char* _env_vars, std::string _image_path) {
      new (this) BabySitterInfo();
      cmd       = _cmd;
      r_limits  = _limits;
      env_vars  = _env_vars;
      image_path = _image_path;
  }
};