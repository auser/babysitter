#include "CppUTest/TestHarness.h"
#include "string_utils.h"

TEST_GROUP(StringTest)
{
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
  LONGS_EQUAL(count_args("hello world and fun"), 4);
}