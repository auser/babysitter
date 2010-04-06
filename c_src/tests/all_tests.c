#include <stdio.h>
#include "minunit.h"
// Tests
#include "ei_decode_test.h"

static char * all_tests() {
  mu_run_test(test_foo);
  mu_run_test(test_bar);
  return 0;
}

int main(int argc, char **argv) {
  char *result = all_tests();
  if (result != 0) {
    printf("%s\n", result);
  }
  else {
    printf("ALL TESTS PASSED\n");
  }
  printf("Tests run: %d\n", tests_run);

  return result != 0;
}
