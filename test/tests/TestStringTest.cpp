#include "CppUTest/TestHarness.h"
#include "string_utils.h"

TEST_GROUP(StringTest)
{
  char **cargv;
  int cargc;
  char *cmd;
  
  void setup()
  {
  }
  void teardown()
  {
  }
};

TEST(StringTest, TestingCountArgs)
{
  LONGS_EQUAL(count_args("hello world and good day"), 5);
  LONGS_EQUAL(count_args(""), 0);
  LONGS_EQUAL(count_args("dogs"), 1);
  LONGS_EQUAL(count_args("hello world and fun"), 4);
}

TEST(StringTest, TestArgify)
{
  cargc = argify("/bin/bash -c ls -l /var/babysitter", &cargv);
  LONGS_EQUAL(cargc, 5);
  STRCMP_EQUAL(cargv[0], "/bin/bash");
  STRCMP_EQUAL(cargv[1], "-c");
  STRCMP_EQUAL(cargv[2], "ls");
  STRCMP_EQUAL(cargv[3], "-l");
  STRCMP_EQUAL(cargv[4], "/var/babysitter");
}

TEST(StringTest, TestCommandify)
{
  cargc = argify("", &cargv);
  LONGS_EQUAL(cargc, -1);
  
  cargc = argify("/bin/bash -c ls -l /var/babysitter", &cargv);
  LONGS_EQUAL(cargc, 5);
  cmd = commandify(cargc, (const char**) cargv);
  STRCMP_EQUAL(cmd, "/bin/bash -c ls -l /var/babysitter");
}

TEST(StringTest, TestChomp)
{
  STRCMP_EQUAL(chomp((char*)"hello world     "), "hello world");
  STRCMP_EQUAL(chomp((char*)" hello world"), "hello world");
  STRCMP_EQUAL(chomp((char*)" hello world "), "hello world");
  STRCMP_EQUAL(chomp((char*)"hello   world "), "hello   world");
}