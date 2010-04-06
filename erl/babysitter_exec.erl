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
  test/0,
  test_pid/1,
  test_args/1
]).

init() ->
  Lib = filename:join([
    filename:dirname(code:which(?MODULE)),
    "..", "priv", "lib", ?MODULE
  ]),
  erlang:load_nif(Lib, 0).

% Exports
test() ->
  "Hello world".
  
% babysitter_exec:test_args([1,2,2]).
test_args(_X) ->
  ?NIF_ERROR_MSG.

% babysitter_exec:test_pid(2).
test_pid(_Pid) ->
  ?NIF_ERROR_MSG.
  