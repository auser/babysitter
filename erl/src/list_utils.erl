-module (list_utils).

-export ([merge_proplists/1]).

merge_proplists(Proplists) -> merge_proplists(Proplists, []).
merge_proplists([], Acc) -> lists:reverse(Acc);
merge_proplists([{Key, Value} = T|Rest], Acc) ->
  M = case proplists:get_value(Key, Acc) of
    undefined -> [T|Acc];
    V ->
      [{Key, lists:flatten([V, Value])}|proplists:delete(Key, Acc)]
  end,
  merge_proplists(Rest, M);
merge_proplists([List|Rest], Acc) when is_list(List) -> merge_proplists(Rest, merge_proplists(List, Acc));
merge_proplists([_Other|Rest], Acc) -> 
  merge_proplists(Rest, Acc).
