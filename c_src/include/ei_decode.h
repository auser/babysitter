#ifndef EI_DECODE_H
#define EI_DECODE_H

#include <erl_nif.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#include "process_manager.h"

/* Defines */
#ifndef MAXATOMLEN
#define MAXATOMLEN 256
#endif

/* Types */
typedef enum _erl_type_ {
  ERL_T_ATOM,
  ERL_T_BINARY,
  ERL_T_EMPTY_LIST,
  ERL_T_FUN,
  ERL_T_PID,
  ERL_T_PORT,
  ERL_T_REF,
  ERL_T_UNKNOWN
} erl_type;

/* Exports */
erl_type decode_erl_type(ErlNifEnv* env, ERL_NIF_TERM term);
const char* erl_type_to_string(erl_type t);
ERL_NIF_TERM error(ErlNifEnv* env, const char *fmt, ...);

#endif
