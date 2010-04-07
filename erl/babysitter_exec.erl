% 
%  babysitter_exec.erl
%  babysitter
%  
%  Created by Ari Lerner on 2010-04-05.
%  Copyright 2010 Ari Lerner. All rights reserved.
% 
-module (babysitter_exec).

-define (NIF_ERROR_MSG, "NIF Library not laoded").

-on_load(init/0).

-export ([
  test_pid/1,
  test_args/2
]).

init() ->
  Lib = filename:join([
    filename:dirname(code:which(?MODULE)),
    "..", "priv", "lib", ?MODULE
  ]),
  erlang:load_nif(Lib, 0).
  
% babysitter_exec:test_args("hello", [{do_before, "ls"},{do_after, "ls -l"}, {env, "HELLO=world"}, {env, "BOBBY=boy"}, {cd, "/var/babysitter/mine/app/123"}]).
-spec(test_args/2 :: (string(), list()) -> any()).
test_args(_X, _Y) -> 
  exit(?NIF_ERROR_MSG).

% babysitter_exec:test_pid(2).
-spec(test_pid/1 :: (integer()) -> any()).
test_pid(_Pid) ->
  exit(?NIF_ERROR_MSG).
