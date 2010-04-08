#include <stdio.h>
#include <string.h>
#include "pm_helpers.h"
#include "minunit.h"
#include "test_helper.h"

char *test_pm_abs_path() {
  mu_assert(pm_abs_path("./bin/path") == 0, "./bin/path is not an absolute path");
  mu_assert(pm_abs_path("/bin/bash") == 0, "/bin/bash is not an absolute path");
  mu_assert(pm_abs_path("binary") == -1, "binary is an absolute path");
  mu_assert(pm_abs_path(" ./binary") == -1, " ./binary is an absolute path");
  return 0;
}

char *test_find_binary() {
  mu_assert(!strcmp(find_binary("/bin/bash"), "/bin/bash"), "/bin/bash was not an absolute path and could not be found");
  mu_assert(!strcmp(find_binary("bash"), "/bin/bash"), "/bin/bash was not an absolute path and could not be found");
  mu_assert(!strcmp(find_binary("made_up_binary"), "\0"), "made_up_binary was found?!? ");
  return 0;
}