-module (babysitter_tests).
-include_lib("eunit/include/eunit.hrl").

-define (TEST_PATH, fun() ->
    TestDir = filename:dirname(filename:dirname(code:which(?MODULE))),
    Dir = filename:join([TestDir, "test", "bin"]),
    {env, lists:flatten(["PATH=/usr/bin:/bin:", Dir])}
  end()).

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
        fun test_listing_processes/0,
        fun test_starting_one_process/0,
        fun test_exec_one_process/0,
        fun test_starting_many_processes/0,
        fun test_killing_a_process/0,
        fun test_killing_a_process_with_erlang_process/0,
        fun test_status_listing_of_a_process/0,
        fun test_running_hooks/0,
        fun test_failed_hooks/0,
        % Babysitter fun tests
        fun test_babysitter_config_actions/0
      ]
    }
  }.

test_starting_one_process() ->
  {ok, _ErlProcess, Pid} = babysitter:bs_spawn_run("test_bin 2.1", [{env, "HELLO=world"}, ?TEST_PATH]),
  CommandArgString = lists:flatten(io_lib:format("ps aux | grep ~p | grep -v grep |awk '{print $2}'", [Pid])),
  ShouldMatch = lists:flatten([erlang:integer_to_list(Pid), "\n"]),
  ?assertCmdOutput(ShouldMatch, CommandArgString).

test_exec_one_process() ->
  {ok, Pid, Status} = babysitter:bs_run("test_bin 1.7", [{env, "TEST=2.3"}, ?TEST_PATH]),
  CommandArgString = lists:flatten(io_lib:format("ps aux | grep ~p | grep -v grep |  wc -l | tr -d ' '", [Pid])),
  ?assertEqual(0, Status),
  ?assertCmdOutput("0\n", CommandArgString).

test_starting_many_processes() ->
  Count = 50,
  Fun = fun() -> babysitter:bs_spawn_run("test_bin 2.2", [{env, "NAME=$NAME"}, {env, "NAME=bob"}, ?TEST_PATH]) end,
  lists:map(fun(_X) -> Fun() end, lists:seq(1, Count)),
  CommandArgString = "ps aux | grep -v grep | grep 'test_bin 2.2' | wc -l | tr -d ' '",
  O = ?cmd(CommandArgString),
  {Int, _} = string:to_integer(O),
  ?assertEqual(Int, 50).

test_killing_a_process() ->
  {ok, _ErlProcess, Pid} = babysitter:bs_spawn_run("test_bin 2.6", [{env, "NAME=ari"}, ?TEST_PATH]),
  {exit_status, Pid, _Status} = babysitter:kill_pid(Pid),
  CommandArgString = lists:flatten(io_lib:format("ps aux | grep ~p | grep -v grep | wc -l | tr -d ' '", [Pid])),
  timer:sleep(500),
  O = ?cmd(CommandArgString),
  {Int, _} = string:to_integer(O),
  ?assertEqual(0, Int).
  
test_killing_a_process_with_erlang_process() ->
  {ok, ErlProcess, Pid} = babysitter:bs_spawn_run("test_bin 201.1", [{env, "NAME=ari"}, ?TEST_PATH]),
  ErlProcess ! {stop},
  timer:sleep(2),
  CommandArgString = lists:flatten(io_lib:format("ps aux | grep ~p | grep -v grep | wc -l | tr -d ' '", [Pid])),
  O = ?cmd(CommandArgString),
  {Int, _} = string:to_integer(O),
  ?assertEqual(0, Int).

test_listing_processes() ->
  Count = 5,
  Fun = fun() -> babysitter:bs_spawn_run("test_bin 2.4", [{env, "NAME=$NAME"}, {env, "NAME=bob"}, ?TEST_PATH]) end,
  Pids = lists:map(fun(_X) -> {ok, _ErlProcess, Pid} = Fun(), Pid end, lists:seq(1, Count)),
  {ok, ListOfPids} = babysitter:list(),
  ?assertEqual(ListOfPids, Pids).

test_status_listing_of_a_process() ->
  {ok, _ErlProcess, Pid} = babysitter:bs_spawn_run("test_bin 10.8", [{env, "HELLO=world"}, ?TEST_PATH]),
  ?assertEqual({ok, Pid, 0}, babysitter:status(Pid)),
  babysitter:kill_pid(Pid).

test_babysitter_config_actions() ->
  {ok, _ErlProcess, Pid} = babysitter:bs_spawn_run("test_bin", [{env, "NAME=ari"}, ?TEST_PATH]),
  ?assert(true == babysitter:running(Pid)),
  babysitter:kill_pid(Pid).

test_running_hooks() ->
  {ok, _ErlProcess, Pid} = babysitter:bs_spawn_run("test_bin 201.2", [{env, "NAME=ari"}, ?TEST_PATH, {do_before, "echo 'hello world' > /tmp/test_running_hooks_file.tmp"}]),
  ?assert(true == babysitter:running(Pid)),
  ?assert(filelib:is_file("/tmp/test_running_hooks_file.tmp")),
  file:delete("/tmp/test_running_hooks_file.tmp"),
  babysitter:kill_pid(Pid).

test_failed_hooks() ->
  {error, State, Pid1, _ExitStatus1, StrOut1, StrError1} = babysitter:bs_spawn_run("test_bin 201.3", [{env, "NAME=ari"}, {do_before, "omgwtfcommanddoesntexist goes here"}, ?TEST_PATH]),
  ?assert(before_command == State),
  ?assertEqual("No such file or directory", StrError1),
  ?assertEqual("/bin/bash: omgwtfcommanddoesntexist: command not found\n", StrOut1),
  ?assert(false == babysitter:running(Pid1)),
  {error, State2, Pid2, _ExitStatus2, _StrOut2, _StrError2} = babysitter:bs_run("test_bin 1.1", [{env, "NAME=ari"}, {do_after, "omgwtfcommanddoesntexist goes here"}, ?TEST_PATH]),
  ?assert(after_command == State2),
  ?assert(false == babysitter:running(Pid2)),
  % % Test command output
  {error, State3, Pid3, _ExitStatus3, _StrOut3, _StrError3} = babysitter:bs_run("thisdoesnt exist either", [{env, "NAME=ari"}, ?TEST_PATH]),
  ?assert(command == State3),
  ?assert(false == babysitter:running(Pid3)),
  passed.