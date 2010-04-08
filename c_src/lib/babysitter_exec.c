#include "babysitter_exec.h"
#include "process_manager.h"
#include "ei_decode.h"

/**
* Test if the pid is up or not
*
* @params
*   {pid, Int}
* @return
*   0 - Alive
*   Else - Not alive
**/
ERL_NIF_TERM test_pid(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
  if (argc != 1) return error(env, "Wrong argument signature");
  
  int n = -1;
  long pid;
  char atom[MAX_BUFFER_SZ];
  const ERL_NIF_TERM* tuple;
  int arity = 2;
  
  if(!enif_get_tuple(env, argv[0], &arity, &tuple)) return -1;
  
  memset(&atom, '\0', sizeof(atom));
  if (!enif_get_atom(env, tuple[0], atom, sizeof(atom)) < 0) return -1;
  
  if (!strncmp(atom, "pid", 3)) {
    enif_get_long(env, tuple[1], &pid);
    if (pid < 1)
      return error(env, "Invalid pid: %d", (int)pid);
    
    n = pm_check_pid_status(pid);
    return enif_make_ulong(env, n);
  } else {
    return error(env, "Wrong signature");
  }
} 

/**
* Test the arguments of the erlang call
*
* @params
*   {Cmd::string(), Options:list()}
*     Options = {do_before, string()} | {do_after, string()} | {env, string()} | {cd, string()}
* @return
*   {pid, Pid:long()} | {error, Reason:string()}
**/
ERL_NIF_TERM test_args(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
  ERL_NIF_TERM erlRes;
  process_t *process = NULL;
  decode_command_call_into_process(env, argc, argv, &process);
  // Do something with the process
  // erlRes = enif_make_atom(env, "ok");
  pid_t pid = getpid(); // Dummy pid
  if (pid >0)
    erlRes = enif_make_tuple2(env, enif_make_atom(env,"pid"), enif_make_ulong(env, pid));
  else
    erlRes = error(env, "failure to launch");
    
  pm_free_process(process);
  return erlRes;
}

static ErlNifFunc nif_funcs[] =
{
  {"test_pid", 1, test_pid},
  {"test_args", 2, test_args},
};

ERL_NIF_INIT(babysitter_exec, nif_funcs, load, reload, upgrade, unload)

/*--- Erlang stuff ---*/
static int load(ErlNifEnv* env, void** priv, ERL_NIF_TERM load_info)
{
  return 0;
}

static int reload(ErlNifEnv* env, void** priv, ERL_NIF_TERM load_info)
{
  return 0;
}

static int upgrade(ErlNifEnv* env, void** priv, void** old_priv, ERL_NIF_TERM load_info)
{
  return 0;
}

static void unload(ErlNifEnv* env, void* priv)
{
  return;
}
