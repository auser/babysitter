#include <erl_nif.h>

#include "ei_decode.h"
#include "process_manager.h"

/**
* Decode the arguments into a new_process
**/
int decode_into_process(process_t **ptr)
{
  // Instantiate a new process
  if (pm_new_process(ptr))
    return -1;
  return 0;
}

/**
* Decode the erlang term into known enum of ERL_T_
**/
erl_type decode_erl_type(ErlNifEnv* env, ERL_NIF_TERM term)
{
       if (!enif_is_atom(env, term)) return ERL_T_ATOM;
  else if (!enif_is_binary(env, term)) return ERL_T_BINARY;
  else if (!enif_is_empty_list(env, term)) return ERL_T_EMPTY_LIST;
  else if (!enif_is_fun(env, term)) return ERL_T_FUN;
  else if (!enif_is_pid(env, term)) return ERL_T_PID;
  else if (!enif_is_port(env, term)) return ERL_T_PORT;
  else if (!enif_is_ref(env, term)) return ERL_T_REF;
  else return ERL_T_UNKNOWN;
}

/**
* Turn an erl_type into a user-readable string
**/
const char* erl_type_to_string(erl_type t)
{
  switch (t) {
    case ERL_T_ATOM: return "atom"; break;
    case ERL_T_BINARY: return "binary"; break;
    case ERL_T_EMPTY_LIST: return "empty_list"; break;
    case ERL_T_FUN: return "function"; break;
    case ERL_T_PID: return "pid"; break;
    case ERL_T_PORT: return "port"; break;
    case ERL_T_REF: return "ref"; break;
    default: return "other";
  }
}

/**
* error
**/
ERL_NIF_TERM error(ErlNifEnv* env, const char *fmt, ...)
{
  char str[MAXATOMLEN];
  va_list vargs;
  va_start (vargs, fmt);
  vsnprintf(str, sizeof(str), fmt, vargs);
  va_end   (vargs);
  
  return enif_make_tuple2(env, enif_make_atom(env,"error"), enif_make_atom(env, str));
}

