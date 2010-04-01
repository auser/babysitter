#include "fs.h"

#include <CppUTest/TestHarness.h>

TEST_GROUP(FileSystem)
{
  void setup() {}
  void teardown() {}
};

TEST(FileSystem, TestingCountArgs)
{
  LONGS_EQUAL(0, dir_size_r("./non/existent/dir"));
}