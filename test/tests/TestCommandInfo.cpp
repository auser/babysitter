#include "command_info.h"

#include <CppUTest/TestHarness.h>

// Stubs

TEST_GROUP(CommandInfoGroup)
{
  void setup() {
  }
  void teardown() {
  }
};

TEST(CommandInfoGroup, initialized)
{  
  CmdInfo ci;
  LONGS_EQUAL(-1, ci.cmd_pid());
  LONGS_EQUAL(-1, ci.kill_cmd_pid());
  LONGS_EQUAL(4, ci.deadline());
  LONGS_EQUAL(false, ci.sigterm());
  LONGS_EQUAL(false, ci.sigkill());
}

TEST(CommandInfoGroup, initialized_with_args)
{
  CmdInfo ci("/bin/bash /usr/bin/daemon", "", 78992);
  STRCMP_EQUAL("/bin/bash /usr/bin/daemon", ci.cmd());
  LONGS_EQUAL(78992, (int)ci.cmd_pid());
  STRCMP_EQUAL("", ci.kill_cmd());
  LONGS_EQUAL(false, ci.sigterm());
  LONGS_EQUAL(false, ci.sigkill());
}