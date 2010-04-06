#include "babysitter_exec.h"
#include "process_manager.h"

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
  int i = 0;
  printf("--- %d args ---\n", argc);
  for(i = 0; i < argc; i++)
    printf("argv[%d] = %s\n", (int)i, erl_type_to_string(argv[i]));
    
  return enif_make_atom(env, erl_type_to_string(argv[0]));
}

static ErlNifFunc nif_funcs[] =
{
  {"test_pid", 1, test_pid},
  {"test_args", 1, test_args}
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
