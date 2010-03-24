#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmockery.h>

// Test cases
#include "test_string_utils.h"

int main(int argc, char* argv[]) {
  // String tests
  const UnitTest string_tests[] = {
    unit_test(count_args_assertions),
    unit_test(argify_assertions),
    unit_test(commandify_assertions),
    unit_test(chomp_assertions),
  };
  
  return run_tests(string_tests);
}