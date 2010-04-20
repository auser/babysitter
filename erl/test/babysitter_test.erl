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
  babysitter:spawn_new("sleep 10", [{env, "HELLO=world"}]),
  receive
    X -> X
  after 1000 ->
    ok
  end,
  ok.