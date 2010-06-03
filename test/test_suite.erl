-module (test_suite).

-include_lib("eunit/include/eunit.hrl").
 
all_test_() ->
  [
    {module, babysitter_tests},
    {module, babysitter_parser_tests},
    {module, babysitter_list_utils_tests},
    {module, babysitter_config_tests},
    {module, babysitter_load_tests}
  ].
