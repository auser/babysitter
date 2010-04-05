% 
%  babysitter_exec.erl
%  babysitter
%  
%  Created by Ari Lerner on 2010-04-05.
%  Copyright 2010 Ari Lerner. All rights reserved.
% 
-module (babysitter_exec).


-on_load(init/0).

-export ([
  test/0,
  test_pid/1
]).

% babysitter_exec:test_pid(2).
init() ->
  Lib = filename:join([
    filename:dirname(code:which(?MODULE)),
    "..", "priv", ?MODULE
  ]),
  erlang:load_nif(Lib, 0).

% Exports
test() ->
  "Hello world".
  
test_pid(_Pid) ->
  "NIF Library not loaded".
  