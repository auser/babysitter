-module (babysitter_test).
-include_lib("eunit/include/eunit.hrl").

setup() ->
  babysitter:start_link([]),
  ok.
  
teardown(_X) ->
  babysitter:stop(),
  ok.

starting_test_() ->
  {spawn,
    {setup,
      fun setup/0,
      fun teardown/1,
      [
        ?_assert({noreply, x} =:= babysitter:spawn_new([{env, "HELLO=world"}, {command, "sleep 10"}]))
      ]
    }
  }.