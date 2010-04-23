-module (babysitter_config_test).
-include_lib("eunit/include/eunit.hrl").

setup() ->
  ok.
  
teardown(_X) ->
  ok.

starting_test_() ->
  {spawn,
    {setup,
      fun setup/0,
      fun teardown/1,
      [
        fun read_test/0
      ]
    }
  }.

read_test() ->
  Dir = filename:dirname(code:which(?MODULE)),
  ConfigDir = filename:join([Dir, "..", "..", "config", "apps"]),
  {ok, Files} = babysitter_config:read(ConfigDir),
  ?assertEqual(Files, ["java.conf", "rack.conf"]).