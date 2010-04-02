#include "babysitter_utils.h"
#include "string_utils.h"
#include "honeycomb.h"

#include <CppUTest/TestHarness.h>

// Stubs
int handle_command_line(char *b, char*c) {return 0;}

TEST_GROUP(BabysitterUtils)
{
  void setup() {}
  void teardown() {}
};

TEST(BabysitterUtils, parse_sha_from_git_directory)
{
  const char *real_sha = "f6f6641d559fa55c3cb1af0e721c3d1373aa58c9";
  const char *git_sha = parse_sha_from_git_directory("../test/fixtures/git_dir", "git_root");
  STRCMP_EQUAL(git_sha, real_sha);
  STRCMP_EQUAL(parse_sha_from_git_directory("./non/existant/directory"), NULL);
}

TEST(BabysitterUtils, parse_the_command_line)
{
  char **argv = {0};
  
  int argc = argify("start --name bob_test --port 90010 --type rack --image bob.img", &argv);
  Honeycomb comb;
  parse_the_command_line_into_honeycomb_config(argc, argv, &comb);
  
  LONGS_EQUAL(9, argc);
  LONGS_EQUAL(90010, (int)comb.port());
  STRCMP_EQUAL("bob_test", comb.name());
  STRCMP_EQUAL("rack", comb.app_type());
  STRCMP_EQUAL("bob.img", comb.image());
  STRCMP_EQUAL("/var/beehive/storage", comb.storage_dir());
}