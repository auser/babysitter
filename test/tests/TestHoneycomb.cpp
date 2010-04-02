#include "honeycomb.h"
#include "string_utils.h"

#include <CppUTest/TestHarness.h>

TEST_GROUP(Honeycomb)
{
  void setup()
  {
  }
  void teardown()
  {
  }
};

TEST(Honeycomb, initialized_defaults)
{
  Honeycomb comb;
  STRCMP_EQUAL("rack", comb.app_type());
  STRCMP_EQUAL("/var/beehive/working", comb.working_dir());
  STRCMP_EQUAL("/var/beehive/run", comb.run_dir());
  STRCMP_EQUAL("/var/beehive/storage", comb.storage_dir());
  STRCMP_EQUAL("", comb.scm_url());
  STRCMP_EQUAL("", comb.skel());
  STRCMP_EQUAL("", comb.sha());
  STRCMP_EQUAL("", comb.image());
  STRCMP_EQUAL("", comb.name());
  
  LONGS_EQUAL(8080, comb.port());
  LONGS_EQUAL(INT_MAX, comb.nice());
  LONGS_EQUAL(-1, (int)comb.user());
  LONGS_EQUAL(-1, (int)comb.group());
}

TEST(Honeycomb, set_root_dir)
{
  Honeycomb comb;
  comb.set_root_dir("/var/other/directory");
  STRCMP_EQUAL("/var/other/directory/working", comb.working_dir());
  STRCMP_EQUAL("/var/other/directory/run", comb.run_dir());
  STRCMP_EQUAL("/var/other/directory/storage", comb.storage_dir());
}

TEST(Honeycomb, set_sha)
{
  Honeycomb comb;
  comb.set_sha("SHASHASHA");
  STRCMP_EQUAL("SHASHASHA", comb.sha());
}

TEST(Honeycomb, add_env_count)
{
  Honeycomb comb;
  LONGS_EQUAL(0, comb.env_size());
  comb.add_env("HAIRS=not nice");
  LONGS_EQUAL(1, comb.env_size());
  comb.add_env("BOXES=cartons");
  LONGS_EQUAL(2, comb.env_size());
  comb.add_env("PRESIDENTS=alive");
  LONGS_EQUAL(3, comb.env_size());
  // char * const *env = comb.env();
  // while(env != NULL) {
  //   STRCMP_EQUAL("HAIRS= not nice", env);
  // }
}