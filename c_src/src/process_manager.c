#include <signal.h>

int pm_check_pid_status(pid_t pid)
{
  if (pid < 1) return -1; // Illegal
  return kill(pid, 0);
}