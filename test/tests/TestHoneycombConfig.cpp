#include "babysitter_types.h"
#include "honeycomb.h"
#include "honeycomb_config.h"
#include "hc_support.h"

#include <CppUTest/TestHarness.h>

TEST_GROUP(HoneycombConfig)
{
  honeycomb_config* config;
  
  void setup()
  {
  }
  void teardown()
  {
  }
};

TEST(HoneycombConfig, initialization)
{
  config = a_new_honeycomb_config_object();
  LONGS_EQUAL(0, config->num_phases);
  STRCMP_EQUAL(NULL, config->app_type);
  STRCMP_EQUAL(NULL, config->root_dir);
  STRCMP_EQUAL(NULL, config->run_dir);
  STRCMP_EQUAL(NULL, config->env);
  STRCMP_EQUAL(NULL, config->stdout);
}

TEST(HoneycombConfig, phase_type_to_string)
{
  STRCMP_EQUAL(phase_type_to_string(T_BUNDLE), "bundle");
  STRCMP_EQUAL(phase_type_to_string(T_START), "start");
  STRCMP_EQUAL(phase_type_to_string(T_MOUNT), "mount");
  STRCMP_EQUAL(phase_type_to_string(T_STOP), "stop");
}

TEST(HoneycombConfig, attribute_type_to_string)
{
  STRCMP_EQUAL(attribute_type_to_string(T_STDOUT), "stdout");
}

TEST(HoneycombConfig, str_to_phase_type)
{
  LONGS_EQUAL(str_to_phase_type((char*)"bundle"), T_BUNDLE);
  LONGS_EQUAL(str_to_phase_type((char*)"milk"), T_UNKNOWN);
}

TEST(HoneycombConfig, collect_to_period)
{
  STRCMP_EQUAL(collect_to_period((char*)"bundle.before"), "bundle");
  STRCMP_EQUAL(collect_to_period((char*)"bundle"), "bundle");
  STRCMP_EQUAL(collect_to_period((char*)""), "");
}