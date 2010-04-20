-module (babysitter_test).
-include_lib("eunit/include/eunit.hrl").

start() ->
  babysitter:start_link([{debug, 4}]),
  ok.
  
stop(_X) ->
  babysitter:stop(),
  ok.

starting_test_() ->
  {spawn,
    {setup,
      fun start/0,
      fun stop/1,
      [
        fun start_test/0
      ]
    }
  }.

% Test starting
start_test() ->
  ?_assert(true).