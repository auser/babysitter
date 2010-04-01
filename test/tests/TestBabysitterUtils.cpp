#include "babysitter_utils.h"

#include <CppUTest/TestHarness.h>

TEST_GROUP(BabysitterUtils)
{
  void setup() {}
  void teardown() {}
};

TEST(BabysitterUtils, parse_sha_from_git_directory)
{
  const char *real_sha = "d5a68aae395b4de3b1c1104cfa621da8a968f9d4";
  const char *git_sha = parse_sha_from_git_directory("../test/fixtures/git_dir");
  STRCMP_EQUAL(git_sha, real_sha);
  STRCMP_EQUAL(parse_sha_from_git_directory("./non/existant/directory"), NULL);
}