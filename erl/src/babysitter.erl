%%%-------------------------------------------------------------------
%%% File    : babysitter.erl
%%% Author  : Ari Lerner
%%% Description : 
%%%
%%% Created :  Thu Dec 24 15:10:38 PST 2009
%%%-------------------------------------------------------------------

-module (babysitter).
-behaviour(gen_server).

%% API
-export ([
  spawn_new/1,
  stop_process/1
]).

-export([start_link/0, start_link/1, stop/0]).

%% gen_server callbacks
-export([init/1, handle_call/3, handle_cast/2, handle_info/2,
         terminate/2, code_change/3]).

% only for tests
-export ([
  build_exec_opts/2,
  build_port_command/1
]).

-define(SERVER, ?MODULE).
-define (PID_MONITOR_TABLE, 'babysitter_mon').

-record(state, {
  port,
  last_trans  = 0,            % Last transaction number sent to port
  trans       = queue:new(),  % Queue of outstanding transactions sent to port
  registry    = ets:new(?PID_MONITOR_TABLE, [protected,named_table]), % Pids to notify when an OsPid exits
  debug       = false
}).

%%====================================================================
%% API
%%====================================================================
spawn_new(Options) -> 
  gen_server:call(?SERVER, {spawn_new, Options}).

stop_process(Arg) ->
  handle_stop_process(Arg).
  
%%--------------------------------------------------------------------
%% Function: start_link() -> {ok,Pid} | ignore | {error,Error}
%% Description: Starts the server
%%--------------------------------------------------------------------
start_link() ->
  gen_server:start_link({local, ?SERVER}, ?MODULE, [], []).

start_link(Options) ->
  gen_server:start_link({local, ?SERVER}, ?MODULE, [Options], []).

stop() ->
  gen_server:call(?SERVER, stop).
%%====================================================================
%% gen_server callbacks
%%====================================================================

%%--------------------------------------------------------------------
%% Function: init(Args) -> {ok, State} |
%%                         {ok, State, Timeout} |
%%                         ignore               |
%%                         {stop, Reason}
%% Description: Initiates the server
%%--------------------------------------------------------------------
init([Options]) ->
  process_flag(trap_exit, true),  
  Exe   = build_port_command(Options),
  Debug = proplists:get_value(verbose, Options, default(verbose)),
  try
    debug(Debug, "exec: port program: ~s\n", [Exe]),
    Port = erlang:open_port({spawn, Exe}, [binary, exit_status, {packet, 2}, nouse_stdio, hide]),
    {ok, #state{port=Port, debug=Debug}}
  catch _:Reason ->
    {stop, io:format("Error starting port '~p': ~200p", [Exe, Reason])}
  end.

%%--------------------------------------------------------------------
%% Function: %% handle_call(Request, From, State) -> {reply, Reply, State} |
%%                                      {reply, Reply, State, Timeout} |
%%                                      {noreply, State} |
%%                                      {noreply, State, Timeout} |
%%                                      {stop, Reason, Reply, State} |
%%                                      {stop, Reason, State}
%% Description: Handling call messages
%%--------------------------------------------------------------------
handle_call({spawn_new, Options}, From, State) ->
  Pid = handle_spawn_new(Options, From, State),
  ets:insert(?PID_MONITOR_TABLE, {Pid, From}),
  {reply, Pid, State};
handle_call(_Request, _From, State) ->
  Reply = ok,
  {reply, Reply, State}.

%%--------------------------------------------------------------------
%% Function: handle_cast(Msg, State) -> {noreply, State} |
%%                                      {noreply, State, Timeout} |
%%                                      {stop, Reason, State}
%% Description: Handling cast messages
%%--------------------------------------------------------------------
handle_cast(_Msg, State) ->
  {noreply, State}.

%%--------------------------------------------------------------------
%% Function: handle_info(Info, State) -> {noreply, State} |
%%                                       {noreply, State, Timeout} |
%%                                       {stop, Reason, State}
%% Description: Handling all non call/cast messages
%%--------------------------------------------------------------------
handle_info({'EXIT', Pid, _Status} = Tuple, State) ->
  io:format("Os process: ~p died~n", [Pid]),
  case ets:lookup(?PID_MONITOR_TABLE, Pid) of
    [{Pid, Caller}] -> 
      ets:delete(?PID_MONITOR_TABLE, Pid),
      Caller ! Tuple;
    _ -> ok
  end,
  {noreply, State};
handle_info(Info, State) ->
  io:format("Info in babysitter: ~p~n", [Info]),
  {noreply, State}.

%%--------------------------------------------------------------------
%% Function: terminate(Reason, State) -> void()
%% Description: This function is called by a gen_server when it is about to
%% terminate. It should be the opposite of Module:init/1 and do any necessary
%% cleaning up. When it returns, the gen_server terminates with Reason.
%% The return value is ignored.
%%--------------------------------------------------------------------
terminate(_Reason, _State) ->
  ok.

%%--------------------------------------------------------------------
%% Func: code_change(OldVsn, State, Extra) -> {ok, NewState}
%% Description: Convert process state when code is changed
%%--------------------------------------------------------------------
code_change(_OldVsn, State, _Extra) ->
  {ok, State}.

%%--------------------------------------------------------------------
%%% Internal functions
%%--------------------------------------------------------------------
handle_spawn_new({port, T}, From, #state{last_trans = LastTrans} = State) ->
  try is_port_command(T, State) of
    {ok, Term, Link} ->
      Next = next_trans(LastTrans),
      io:format("{~p, ~p}~n", [Next, Term]),
      erlang:port_command(State#state.port, term_to_binary({Next, Term})),
      {noreply, State#state{trans = queue:in({Next, From, Link}, State#state.trans)}}
    catch _:{error, Why} ->
      {reply, {error, Why}, State}
  end;
handle_spawn_new(T, _From, _State) ->
  io:format("handle_spawn_new got: ~w~n", [T]),
  ok.

handle_stop_process(Pid) when is_pid(Pid) ->
  ok.
  

%%-------------------------------------------------------------------------
%% @spec () -> Default::exec_options()
%% @doc Provide default value of a given option.
%% @end
%%-------------------------------------------------------------------------
default() -> 
    [{debug, false},    % Debug mode of the port program. 
     {verbose, false},  % Verbose print of events on the Erlang side.
     {port_program, default(port_program)}].

default(port_program) -> 
    % Get architecture (e.g. i386-linux)
    Dir = filename:dirname(filename:dirname(code:which(?MODULE))),
    filename:join([Dir, "priv", "bin", "babysitter"]);
default(Option) ->
  search_for_application_value(Option).

% Look for it at the environment level first
search_for_application_value(Param) ->
  case application:get_env(babysitter, Param) of
    undefined         -> search_for_application_value_from_environment(Param);
    {ok, undefined}   -> search_for_application_value_from_environment(Param);
    {ok, V}    -> V
  end.

search_for_application_value_from_environment(Param) ->
  EnvParam = string:to_upper(erlang:atom_to_list(Param)),
  case os:getenv(EnvParam) of
    false -> proplists:get_value(Param, default());
    E -> E
  end.


% Build the port process command
build_port_command(Opts) ->
  Args = build_port_command1(Opts, []),
  proplists:get_value(port_program, Opts, default(port_program)) ++ lists:flatten([" -n"|Args]).

% Fold down the option list and collect the options for the port program
build_port_command1([], Acc) -> Acc;
build_port_command1([{verbose, V} = T | Rest], Acc) -> build_port_command1(Rest, [port_command_option(T) | Acc]);
build_port_command1([{debug, X} = T|Rest], Acc) when is_integer(X) -> 
  build_port_command1(Rest, [port_command_option(T) | Acc]);
build_port_command1([{K,V}|Rest], Acc) -> build_port_command1(Rest, Acc).

% Purely to clean this up
port_command_option({debug, X}) when is_integer(X) -> io:fwrite(" --debug ~w", [X]);
port_command_option({debug, _Else}) -> " -- debug 4";
port_command_option(_) -> "".

% Build the command-line option
build_cli_option(Switch, Param, Opts) -> 
  case fetch_value(Param, Opts) of
    [] -> [];
    undefined -> [];
    E -> lists:flatten([" ", Switch, " ", E])
  end.

% Accept only know execution options
% Throw the rest away
build_exec_opts([], Acc) -> Acc;
build_exec_opts([{cd, _V}=T|Rest], Acc) -> build_exec_opts(Rest, [T|Acc]);
build_exec_opts([{env, _V}=T|Rest], Acc) -> build_exec_opts(Rest, [T|Acc]);
build_exec_opts([{nice, _V}=T|Rest], Acc) -> build_exec_opts(Rest, [T|Acc]);
build_exec_opts([{stdout, _V}=T|Rest], Acc) -> build_exec_opts(Rest, [T|Acc]);
build_exec_opts([{stderr, _V}=T|Rest], Acc) -> build_exec_opts(Rest, [T|Acc]);
build_exec_opts([_Else|Rest], Acc) -> build_exec_opts(Rest, Acc).

% Fetch values and defaults
fetch_value(env, Opts) -> opt_or_default(env, [], Opts);
fetch_value(cd, Opts) -> opt_or_default(cd, undefined, Opts);
fetch_value(skel, Opts) -> opt_or_default(skel, undefined, Opts);
fetch_value(start_command, Opts) -> opt_or_default(start_command, "thin -- -R config.ru start", Opts).

opt_or_default(Param, Default, Opts) ->
  case proplists:get_value(Param, Opts) of
    undefined -> Default;
    V -> V
  end.

% PRIVATE
debug(false, _, _) ->     ok;
debug(true, Fmt, Args) -> io:format(Fmt, Args).

erlang_daemon_command() ->
  Dir = filename:dirname(filename:dirname(code:which(?MODULE))),
  filename:join([Dir, "priv", "bin", "babysitter"]).

% Check if the call is a legal maneuver
is_port_command({start, {run, _Cmd, Options} = T, Link}, State) ->
  check_cmd_options(Options, State),
  {ok, T, Link};
is_port_command({list} = T, _State) -> 
  {ok, T, undefined};
is_port_command({stop, OsPid}=T, _State) when is_integer(OsPid) -> 
  {ok, T, undefined};
is_port_command({stop, Pid}, _State) when is_pid(Pid) ->
  case ets:lookup(exec_mon, Pid) of
    [{Pid, OsPid}]  -> {ok, {stop, OsPid}, undefined};
    []              -> throw({error, no_process})
  end;
is_port_command({kill, OsPid, Sig}=T, _State) when is_integer(OsPid),is_integer(Sig) -> 
  {ok, T, undefined};
is_port_command({kill, Pid, Sig}, _State) when is_pid(Pid),is_integer(Sig) -> 
  case ets:lookup(exec_mon, Pid) of
    [{Pid, OsPid}]  -> {ok, {kill, OsPid, Sig}, undefined};
    []              -> throw({error, no_process})
  end.

% Check on the command options
check_cmd_options([{cd, Dir}|T], State) when is_list(Dir) ->
  check_cmd_options(T, State);
check_cmd_options([{env, Env}|T], State) when is_list(Env) ->
  check_cmd_options(T, State);
check_cmd_options([{do_before, Cmd}|T], State) when is_list(Cmd) ->
  check_cmd_options(T, State);
check_cmd_options([{do_after, Cmd}|T], State) when is_list(Cmd) ->
  check_cmd_options(T, State);
check_cmd_options([{nice, I}|T], State) when is_integer(I), I >= -20, I =< 20 ->
  check_cmd_options(T, State);
check_cmd_options([{Std, I}|T], State) when Std=:=stderr, I=/=Std; Std=:=stdout, I=/=Std ->
  if I=:=null; I=:=stderr; I=:=stdout; is_list(I); 
    is_tuple(I), tuple_size(I)=:=2, element(1,I)=:="append", is_list(element(2,I)) ->  
      check_cmd_options(T, State);
  true -> 
    throw({error, io:format("Invalid ~w option ~p", [Std, I])})
  end;
check_cmd_options([Other|_], _State) -> throw({error, {invalid_option, Other}});
check_cmd_options([], _State)        -> ok.


% So that we can get a unique id for each communication
next_trans(I) when I < 268435455 -> I+1;
next_trans(_) -> 1.