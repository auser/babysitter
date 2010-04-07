#include "babysitter_exec.h"
#include "process_manager.h"
#include "ei_decode.h"

/**
* Test if the pid is up or not
*
* @params
*   {test_pid, Int}
* @return
*   1 - Alive
*   -1 - Not alive
**/
ERL_NIF_TERM test_pid(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
  return enif_make_string(env, "Hello world!", ERL_NIF_LATIN1); 
}

ERL_NIF_TERM test_args(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
  ERL_NIF_TERM erlRes;
  process_t *process = NULL;
  decode_command_call_into_process(env, argc, argv, &process);
  
  printf("process: %p (%d)\n", process, process->env_c);
  
  // erlRes = enif_make_string(env, process->command, ERL_NIF_LATIN1);
  erlRes = enif_make_atom(env, "ok");
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
