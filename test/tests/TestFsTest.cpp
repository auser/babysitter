#include "CppUTest/TestHarness.h"
#include "fs.h"

TEST_GROUP(FS)
{
  void setup() {}
  void teardown() {}
}

TEST(FS, TestMkdirP)
{
  printf("hi\n");
}