#include <stdio.h>
#include <string.h>
#include "process_manager.h"
#include "minunit.h"
#include "test_helper.h"

char *test_new_process() {
  process_t *test_process = NULL;
  pm_new_process(&test_process);
  mu_assert(test_process->env_c == 0, "processes env count is not zero when initialized");
  mu_assert(test_process->env == NULL, "processes env is not NULL when initialized");
  mu_assert(test_process->command == NULL, "processes command is not NULL when initialized");
  
  pm_free_process(test_process); return 0;
}

char *test_pm_check_pid_status() {
  mu_assert(pm_check_pid_status(-2) == -1, "can send a signal to a process lower than 1");
  mu_assert(pm_check_pid_status(1000) != 0, "can send a signal to a process lower than 1");
  return 0;
}

char *test_pm_valid_process() {
  process_t *test_process = NULL;
  pm_new_process(&test_process);
  
  mu_assert(pm_process_valid(&test_process) == -1, "process shouldn't be valid, but is");
  mu_assert(!pm_malloc_and_set_attribute(&test_process->command, "ls -l"), "copy command failed");
  mu_assert(pm_process_valid(&test_process) == 0, "process should be valid");
  
  pm_free_process(test_process); return 0;
}

char *test_pm_malloc_and_set_attribute() {
  process_t *test_process = NULL;
  pm_new_process(&test_process);
  
  mu_assert(!pm_malloc_and_set_attribute(&test_process->command, "ls -l"), "copy command failed");
  mu_assert(!strcmp(test_process->command, "ls -l"), "copy command copied the value properly");
  
  pm_free_process(test_process); return 0;
}

char *test_pm_add_env() {
  process_t *test_process = NULL;
  pm_new_process(&test_process);
  
  mu_assert(test_process->env_c == 0, "processes env count is not zero when initialized");
  mu_assert(test_process->env == NULL, "processes env is not NULL when initialized");
  
  pm_add_env(&test_process, "BOB=sally");
  mu_assert(test_process->env_c == 1, "the environment variable did not copy properly");
  mu_assert(!strcmp(test_process->env[0], "BOB=sally"), "the environment variable did not copy properly");
  
  pm_add_env(&test_process, "NICK=name");  
  mu_assert(test_process->env_c == 2, "the environment variable did not copy properly");
  mu_assert(!strcmp(test_process->env[0], "BOB=sally"), test_process->env[0]);
  mu_assert(!strcmp(test_process->env[1], "NICK=name"), test_process->env[1]);
  
  pm_add_env(&test_process, "ARI=awesome");  
  mu_assert(test_process->env_c == 3, "the environment variable did not copy properly");
  mu_assert(!strcmp(test_process->env[0], "BOB=sally"), test_process->env[0]);
  mu_assert(!strcmp(test_process->env[1], "NICK=name"), test_process->env[1]);
  mu_assert(!strcmp(test_process->env[2], "ARI=awesome"), test_process->env[2]);
  // pm_add_env
  pm_free_process(test_process); return 0;
}