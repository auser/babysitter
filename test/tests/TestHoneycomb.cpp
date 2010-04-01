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
  LONGS_EQUAL(1, parse_config_dir("/non/existant/directory", known_configs));
}

TEST(HoneycombConfig, parse_config_dir)
{
  LONGS_EQUAL(0, parse_config_dir("../test/fixtures/configs", known_configs));
  LONGS_EQUAL(2, known_configs.size());
  ConfigMapT::iterator it;
  it = known_configs.find("rack");
  honeycomb_config *rack = it->second;
  
  STRCMP_EQUAL("$STORAGE_DIR/$APP_NAME/$BEE_SHA.tgz", rack->image);
  STRCMP_EQUAL("../test/fixtures/configs/rack.conf", rack->filepath);
  STRCMP_EQUAL("ruby /usr/bin/irb /usr/bin/gem thin uuidgen", rack->executables);
  
  LONGS_EQUAL(6, rack->num_phases);
}