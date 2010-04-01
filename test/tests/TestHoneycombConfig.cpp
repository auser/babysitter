#include "babysitter_types.h"
#include "honeycomb.h"
#include "honeycomb_config.h"
#include "hc_support.h"

#include <CppUTest/TestHarness.h>

TEST_GROUP(HoneycombConfig)
{
  honeycomb_config* config;
  phase_t *phase;
  
  void setup()
  {
    config = a_new_honeycomb_config_object();
    phase = new_phase(T_BUNDLE);
  }
  void teardown()
  {
    free_config(config);
    free_phase(phase);
  }
};

TEST(HoneycombConfig, initialization)
{
  LONGS_EQUAL(0, config->num_phases);
  STRCMP_EQUAL(NULL, config->app_type);
  STRCMP_EQUAL(NULL, config->root_dir);
  STRCMP_EQUAL(NULL, config->run_dir);
  STRCMP_EQUAL(NULL, config->env);
  STRCMP_EQUAL(NULL, config->stdout);
}

TEST(HoneycombConfig, add_attribute)
{
  STRCMP_EQUAL(NULL, config->filepath);
  add_attribute(config, T_FILEPATH, (char*)"./a_file");
  STRCMP_EQUAL((char*)"./a_file", config->filepath);
  
  STRCMP_EQUAL(NULL, config->stdout);
  add_attribute(config, T_STDOUT, (char*)">/dev/null");
  STRCMP_EQUAL((char*)">/dev/null", config->stdout);
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

TEST(HoneycombConfig, new_phase)
{
  STRCMP_EQUAL(NULL, phase->before);
  STRCMP_EQUAL(NULL, phase->command);
  STRCMP_EQUAL(NULL, phase->after);
  LONGS_EQUAL(T_BUNDLE, phase->type);
  free_phase(phase);
}