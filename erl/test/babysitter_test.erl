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
        fun test_starting/0
      ]
    }
  }.

test_starting() ->
  {ok, Pid} = babysitter:spawn_new("sleep 3", [{env, "HELLO=world"}]),
  CommandArgString = "ps aux | grep sleep | grep -v grep | awk '{print $2}'",
  ShouldMatch = lists:flatten([erlang:integer_to_list(Pid), "\n"]),
  ?assertCmdOutput(ShouldMatch, CommandArgString).
  