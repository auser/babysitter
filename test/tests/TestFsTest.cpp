#include "fs.h"

#include <CppUTest/TestHarness.h>

TEST_GROUP(FileSystem)
{
  void setup() {}
  void teardown() {}
};

TEST(FileSystem, TestingCountArgs)
{
  int size = dir_size_r("../test/fixtures/test_dir");
  LONGS_EQUAL(1461, size);
}