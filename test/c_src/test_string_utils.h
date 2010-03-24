#include "string_utils.h"

void count_args_assertions(void **state) {
  assert_int_equal(count_args("hello world and good day"), 5);
  assert_int_equal(count_args("hello world and good"), 4);
  assert_int_equal(count_args(""), 0);
  assert_int_equal(count_args("dogs"), 1);
}

void argify_assertions(void **state) {
  char **cargv = {0};
  int cargc = 0;
  
  cargc = argify("/bin/bash -c ls -l /var/babysitter", &cargv);
  assert_int_equal(cargc, 5);
  assert_string_equal(cargv[0], "/bin/bash");
  assert_string_equal(cargv[1], "-c");
  assert_string_equal(cargv[2], "ls");
  assert_string_equal(cargv[3], "-l");
  assert_string_equal(cargv[4], "/var/babysitter");
}

void commandify_assertions(void **state) {
  char **cargv;
  int cargc = 0;
  char *cmd;
  
  cargc = argify("", &cargv);
  assert_int_equal(cargc, -1);
  
  cargc = argify("/bin/bash -c ls -l /var/babysitter", &cargv);
  assert_int_equal(cargc, 5);
  cmd = commandify(cargc, (const char**) cargv);
  assert_string_equal(cmd, "/bin/bash -c ls -l /var/babysitter");
}

void chomp_assertions(void **state) {
  assert_string_equal(chomp("hello world     "), "hello world");
  assert_string_equal(chomp(" hello world"), "hello world");
  assert_string_equal(chomp(" hello world "), "hello world");
  assert_string_equal(chomp("hello   world "), "hello   world");
}