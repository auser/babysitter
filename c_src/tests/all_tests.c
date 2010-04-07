#include <stdio.h>
#include "minunit.h"
// Tests
#include "test_helper.h"
#include "ei_decode_test.h"
#include "process_manager_test.h"

static char * all_tests() {
  mu_run_test(test_new_process);
  mu_run_test(test_pm_check_pid_status);
  mu_run_test(test_pm_malloc_and_set_attribute);
  mu_run_test(test_pm_add_env);
  return 0;
}

int main(int argc, char **argv) {
  char *result = all_tests();
  if (result != 0) {
    printf("[error] %s\n", result);
  }
  else {
    printf("ALL TESTS PASSED\n");
  }
  printf("Tests failed: %d\n", tests_failed);
  printf("Tests run: %d\n", tests_run);
  
  return result != 0;
}
