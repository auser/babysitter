#include "babysitter_types.h"
#include "honeycomb.h"
#include "honeycomb_config.h"
#include "hc_support.h"

#include <CppUTest/TestHarness.h>

TEST_GROUP(HoneycombConfig)
{
  honeycomb_config* config;
  phase_t *phase;
  ConfigMapT  known_configs;
  
  void setup()
  {
    a_new_honeycomb_config_object(&config);
    phase = new_phase(T_BUNDLE);
  }
  void teardown()
  {
    free_config(config);
    free_phase(phase);
  }
};

TEST(HoneycombConfig, parse_config_dir_fails_on_non_existant_dir)
{
  LONGS_EQUAL(parse_config_dir("/non/existant/directory", known_configs), 1);
}

TEST(HoneycombConfig, parse_config_dir)
{
  LONGS_EQUAL(parse_config_dir("../test/fixtures/configs", known_configs), 1);
}