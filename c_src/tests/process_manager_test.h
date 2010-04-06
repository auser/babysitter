#include <stdio.h>
#include "process_manager.h"
#include "minunit.h"
#include "test_helper.h"

char *test_new_process() {
  process_t *test_process = NULL;
  pm_new_process(&test_process);
  mu_assert(test_process->env_c == 0, "processes env count is not zero when initialized");
  mu_assert(test_process->env == NULL, "processes env is not NULL when initialized");
  mu_assert(test_process->command == NULL, "processes command is not NULL when initialized");
  return 0;
}

char *test_pm_check_pid_status() {
  mu_assert(pm_check_pid_status(-2) == -1, "can send a signal to a process lower than 1");
  mu_assert(pm_check_pid_status(1000) != 0, "can send a signal to a process lower than 1");
  return 0;
}