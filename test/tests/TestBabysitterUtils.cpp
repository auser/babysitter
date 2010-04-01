#include "babysitter_utils.h"
#include "string_utils.h"

#include <CppUTest/TestHarness.h>

// Stubs
int handle_command_line(char *b, char*c) {return 0;}
// Externs
extern std::string config_file_dir;                // The directory containing configs
extern std::string root_dir;                       // The root directory to work from within
extern std::string run_dir;                        // The directory to run the bees
extern std::string working_dir;                    // Working directory
extern std::string storage_dir;                    // Storage dir
extern std::string sha;                            // The sha
extern std::string name;                           // Name
extern std::string scm_url;                        // The scm url
extern std::string image;                          // The image to mount
extern string_set  execs;                          // Executables to add
extern string_set  files;                          // Files to add
extern string_set  dirs;                           // Dirs to add
extern phase_type  action;
extern int         port;                           // Port to run

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

TEST(BabysitterUtils, parse_the_command_line)
{
  char **argv = {0};
  
  int argc = argify("start --name bob_test --port 90010 --type rack", &argv);
  parse_the_command_line(argc, argv, 2);
  
  LONGS_EQUAL(7, argc);
  LONGS_EQUAL(port, 90010);
}