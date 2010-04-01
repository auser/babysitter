#include "babysitter_utils.h"

#include <CppUTest/TestHarness.h>

TEST_GROUP(BabysitterUtils)
{
  void setup() {}
  void teardown() {}
};

TEST(BabysitterUtils, parse_sha_from_git_directory)
{
  const char *real_sha = "f6f6641d559fa55c3cb1af0e721c3d1373aa58c9";
  const char *git_sha = parse_sha_from_git_directory("../test/fixtures/git_dir");
  STRCMP_EQUAL(git_sha, real_sha);
  STRCMP_EQUAL(parse_sha_from_git_directory("./non/existant/directory"), NULL);
}