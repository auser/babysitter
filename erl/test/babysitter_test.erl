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
        fun test_starting_one_process/0,
        fun test_starting_many_processes/0,
        fun test_receive_notification_on_death_of_a_process/0
      ]
    }
  }.

test_starting_one_process() ->
  {ok, _ErlProcess, Pid} = babysitter:spawn_new("sleep 2.1", [{env, "HELLO=world"}]),
  CommandArgString = "ps aux | grep 'sleep 2.1' | grep -v grep | awk '{print $2}'",
  ShouldMatch = lists:flatten([erlang:integer_to_list(Pid), "\n"]),
  ?assertCmdOutput(ShouldMatch, CommandArgString).

test_starting_many_processes() ->
  Count = 200,
  Fun = fun() -> babysitter:spawn_new("sleep 2.2", [{env, "NAME=$NAME"}, {env, "NAME=bob"}]) end,
  lists:map(fun(_X) -> Fun() end, lists:seq(1, Count)),
  CommandArgString = "ps aux | grep -v grep | grep 'sleep 2.2' | wc -l | tr -d ' '",
  O = ?cmd(CommandArgString),
  {Int, _} = string:to_integer(O),
  ?assertEqual(Int, 200).

test_receive_notification_on_death_of_a_process() ->
  {ok, _ErlProcess, Pid} = babysitter:spawn_new("sleep 200.1", [{env, "NAME=ari"}]),
  babysitter:kill_pid(Pid),
  CommandArgString = lists:flatten(io_lib:format("ps aux | grep ~p | grep -v grep | wc -l | tr -d ' '", [Pid])),
  O = ?cmd(CommandArgString),
  {Int, _} = string:to_integer(O),
  ?assertEqual(0, Int).