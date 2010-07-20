-module (babysitter_tests).
-include_lib("eunit/include/eunit.hrl").

-define (TEST_PATH, fun() ->
    TestDir = filename:dirname(filename:dirname(code:which(?MODULE))),
    Dir = filename:join([TestDir, "test", "bin"]),
    {env, lists:flatten(["PATH=/usr/bin:/bin:", Dir])}
  end()).

setup() ->
  % application:start(sasl),
  babysitter:start_link([]),
  ok.
  
teardown(_X) ->
  babysitter:stop(),
  ok.

starting_test_() ->
  {inorder,
    {setup,
      fun setup/0,
      fun teardown/1,
      [
        fun test_listing_processes/0,
        fun test_starting_one_process/0,
        fun test_exec_one_process/0,
        fun test_stdout_stderr_redirection/0,
        fun test_stopping_a_process/0,
        fun test_starting_many_processes/0,
        fun test_killing_a_process/0,
        fun test_killing_a_background_process/0,
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
  {ok, _ErlProcess, Pid} = babysitter:bs_spawn_run("test_bin 1.1", [{env, "HELLO=world"}, ?TEST_PATH]),
  CommandArgString = lists:flatten(io_lib:format("ps aux | grep ~p | grep -v grep |awk '{print $2}'", [Pid])),
  ShouldMatch = lists:flatten([erlang:integer_to_list(Pid), "\n"]),
  ?assertCmdOutput(ShouldMatch, CommandArgString),
  kill_all_started_processes([Pid]).

test_exec_one_process() ->
  {ok, Pid, Status} = babysitter:bs_run("test_bin 1.7", [{env, "TEST=2.3"}, ?TEST_PATH]),
  CommandArgString = lists:flatten(io_lib:format("ps aux | grep ~p | grep -v grep |  wc -l | tr -d ' '", [Pid])),
  ?assertEqual(0, Status),
  ?assertCmdOutput("0\n", CommandArgString).

test_stopping_a_process() ->
  {ok, ErlProcess, Pid} = babysitter:bs_spawn_run("test_bin 40.6", [{env, "NAME=ari"}, ?TEST_PATH]),
  ?assertCmdOutput(
    "1\n", 
    lists:flatten(io_lib:format("ps aux | grep ~p | grep -v grep |  wc -l | tr -d ' '", [Pid]))
  ),
  ErlProcess ! {stop},
  timer:sleep(200),
  ?assertCmdOutput(
    "0\n", 
    lists:flatten(io_lib:format("ps aux | grep ~p | grep -v grep |  wc -l | tr -d ' '", [Pid]))
  ),
  passed.

test_starting_many_processes() ->
  Count = 50,
  Fun = fun() -> babysitter:bs_spawn_run("test_bin 10.2", [{env, "NAME=$NAME"}, {env, "NAME=bob"}, ?TEST_PATH]) end,
  lists:map(fun(_X) -> Fun() end, lists:seq(1, Count)),
  CommandArgString = "ps aux | grep -v grep | grep 'test_bin 10.2' | wc -l | tr -d ' '",
  O = ?cmd(CommandArgString),
  {Int, _} = string:to_integer(O),
  ?assertEqual(50, Int).

test_killing_a_process() ->
  {ok, _ErlProcess, Pid} = babysitter:bs_spawn_run("test_bin 1.6", [{env, "NAME=ari"}, ?TEST_PATH]),
  {exit_status, Pid, _Status} = babysitter:kill_pid(Pid),
  CommandArgString = lists:flatten(io_lib:format("ps aux | grep ~p | grep -v grep | wc -l | tr -d ' '", [Pid])),
  timer:sleep(900),
  O = ?cmd(CommandArgString),
  {Int, _} = string:to_integer(O),
  ?assertEqual(0, Int).

test_killing_a_background_process() ->
  {ok, _ErlProcess, Pid} = babysitter:bs_spawn_run("test_bin 100.1 &", [{env, "NAME=ari"}, ?TEST_PATH]),
  {exit_status, Pid, _Status} = babysitter:kill_pid(Pid),
  CommandArgString = lists:flatten(io_lib:format("ps aux | grep ~p | grep -v grep | wc -l | tr -d ' '", [Pid])),
  timer:sleep(1000),
  O = ?cmd(CommandArgString),
  {Int, _} = string:to_integer(O),
  ?assertEqual(0, Int),
  passed.  

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
  StdErrFile = "/tmp/test_failed_hooks.err",
  {error, State, Pid1, _ExitStatus1, StrOut1, StrError1} = babysitter:bs_spawn_run("test_bin 201.3", [{env, "NAME=ari"}, {do_before, "omgwtfcommanddoesntexist goes here"}, ?TEST_PATH, {stderr, StdErrFile}]),
  ?assert(before_command == State),
  ?assertEqual("", StrOut1),
  ?assertEqual("/bin/bash: omgwtfcommanddoesntexist: command not found\n", StrError1),
  ?assert(false == babysitter:running(Pid1)),
  {error, State2, Pid2, _ExitStatus2, _StrOut2, _StrError2} = babysitter:bs_run("test_bin 1.1", [{env, "NAME=ari"}, {do_after, "omgwtfcommanddoesntexist goes here"}, ?TEST_PATH]),
  ?assert(after_command == State2),
  ?assert(false == babysitter:running(Pid2)),
  % % Test command output
  {error, State3, Pid3, _ExitStatus3, _StrOut3, _StrError3} = babysitter:bs_run("thisdoesnt exist either", [{env, "NAME=ari"}, ?TEST_PATH]),
  ?assert(command == State3),
  ?assert(false == babysitter:running(Pid3)),
  file:delete(StdErrFile),
  passed.

test_stdout_stderr_redirection() ->
  StdFile = "/tmp/test_bin.out",
  StdErr  = "/tmp/test_bin.err",
  delete_file([StdFile, StdErr]),
  {ok, _Pid1, ExitStatus1} = babysitter:bs_run("test_bin 2.2", [{env, "NAME=ari"}, ?TEST_PATH, {stdout, StdFile}]),
  ?assert(ExitStatus1 =:= 0),
  % Confirm the stdout is put out right
  {ok, Bin} = file:read_file(StdFile),
  StdoutList = binary_to_list(Bin),
  ?assert(StdoutList =:= "Testing from bin\n"),
  % Do stderr too
  babysitter:bs_run("test_bin -1", [{env, "NAME=ari"}, ?TEST_PATH, {stderr, StdErr}]),
  {ok, Bin2} = file:read_file(StdErr),
  StderrList = binary_to_list(Bin2),
  ?assert(StderrList =:= "BROKEN\n"),
  passed.

delete_file([]) -> ok;
delete_file([File|Rest]) ->
  case filelib:is_file(File) of
    true -> file:delete(File);
    false -> ok
  end,
  delete_file(Rest).

kill_all_started_processes([]) -> ok;
kill_all_started_processes([_Pid|Rest]) ->
  kill_all_started_processes(Rest).