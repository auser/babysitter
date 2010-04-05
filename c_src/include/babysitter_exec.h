
/* Erlang exports */
static int load(ErlNifEnv* env, void** priv, ERL_NIF_TERM load_info);
static int reload(ErlNifEnv* env, void** priv, ERL_NIF_TERM load_info);
static int upgrade(ErlNifEnv* env, void** priv, void** old_priv, ERL_NIF_TERM load_info);
static void unload(ErlNifEnv* env, void* priv);

/* Babysitter exports */
static ERL_NIF_TERM test_pid(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]);