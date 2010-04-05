#include "erl_nif.h"
 
#include <errno.h>
#include <assert.h>

#include "babysitter_exec.h"

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

static ERL_NIF_TERM test_pid(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
  return enif_make_string(env, "Hello world!", ERL_NIF_LATIN1); 
}

static ErlNifFunc nif_funcs[] =
{
  {"test_pid", 1, test_pid}
};

ERL_NIF_INIT(babysitter, nif_funcs, load, reload, upgrade, unload)
