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
  babysitter:spawn_new("sleep 3", [{env, "HELLO=world"}]),
  ArgString = "ps aux | grep sleep | grep -v grep | awk '{print $2}'",
  O = ?cmd(ArgString),
  {Int, _Rest} = string:to_integer(O),
  erlang:display(Int),
  ?assertCmdOutput(Int ++ "\n", ArgString).
  