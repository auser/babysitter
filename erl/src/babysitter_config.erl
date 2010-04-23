%%% babysitter_config_srv.erl
%% @author Ari Lerner <arilerner@mac.com>
%% @copyright 04/22/10 Ari Lerner <arilerner@mac.com>
%% @doc The babysitter configuration
-module (babysitter_config).

-export ([read/1]).

-define (BABYSITTER_CONFIG_DB, 'babysitter_config_db').
-define (CONF_EXTENSION, ".conf").

%%-------------------------------------------------------------------
%% @spec (Dir::list()) -> {ok, FileNames}
%% @doc Take a directory of files and read and parse them into the 
%%      ets database.
%% @end
%%-------------------------------------------------------------------
read(Dir) ->
  case filelib:is_dir(Dir) of
    true -> read_dir(Dir);
    false ->
      case filelib:is_file(Dir) of
        true -> read_files([Dir], []);
        false -> throw({badarg, "Argument must be a file or a directory"})
      end
  end.

%%-------------------------------------------------------------------
%% @spec (Dir::list()) -> {ok, Files::list()}
%% @doc Read a directory and parse the the conf files
%%      
%% @end
%%-------------------------------------------------------------------
read_dir(Dir) ->
  Files = lists:filter(fun(F) -> filelib:is_file(F) end, filelib:wildcard(lists:flatten([Dir, "/*"]))),
  read_files(Files, []).

read_files([], Acc) -> {ok, lists:reverse(Acc)};
read_files([File|Rest], Acc) ->
  ok = parse_config_file(File),
  read_files(Rest, [filename:basename(File)|Acc]).

parse_config_file(Filepath) ->
  X = babysitter_config_parser:file(Filepath),
  erlang:display(X),
  ok.