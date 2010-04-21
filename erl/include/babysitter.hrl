-define (PID_MONITOR_TABLE, 'babysitter_mon').

-define (DBG (Bool, Fmt, Args),
  case Bool of
    true -> io:format(Fmt, Args);
    false -> ok
  end).
  
-record(state, {
  port,
  last_trans  = 0,            % Last transaction number sent to port
  trans       = queue:new(),  % Queue of outstanding transactions sent to port
  registry    = ets:new(?PID_MONITOR_TABLE, [protected,named_table]), % Pids to notify when an OsPid exits
  debug       = false
}).
