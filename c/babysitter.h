#include "honeycomb.h"

//-------------------------------------------------------------------------
// Helpers
//-------------------------------------------------------------------------
const std::string HELP_MESSAGE =
"Usage:\n"
" babysitter [-hnD] [-a N]\n"
"Options:\n"
" -h                Show this message\n"
" -n                Using marshaling file descriptors 3 and 4 instead of default 0 and 1\n"
" -D                Turn on debugging\n"
" -u [username]     Run as this user (if running as root)\n"
" -a [seconds]      Set the number of seconds to live after receiving a SIGTERM/SIGINT (default 30)\n"
" -b [path]         Path to base all the chroot environments on (not yet functional)\n"
;

void  setup_defaults();
ConfigParser config_parser();