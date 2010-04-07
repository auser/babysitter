#ifndef EI_DECODE_H
#define EI_DECODE_H

#include <ei.h>
#include <erl_nif.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "process_manager.h"

/* Defines */
#ifndef MAXATOMLEN
#define MAXATOMLEN 256
#endif

#ifndef BUFFER_SZ
#define BUFFER_SZ 1024
#endif

#ifndef MAX_BUFFER_SZ
#define MAX_BUFFER_SZ 4096
#endif

#ifndef PREFIX_LEN
#define PREFIX_LEN 8
#endif


#define NEW_FLOAT_EXT 'F'
typedef enum _erl_type_ {
  etSmallInt    = ERL_SMALL_INTEGER_EXT // 'a'
 ,etInt         = ERL_INTEGER_EXT       // 'b'
 ,etFloatOld    = ERL_FLOAT_EXT         // 'c'
 ,etFloat       = NEW_FLOAT_EXT         // 'F'
 ,etAtom        = ERL_ATOM_EXT          // 'd'
 ,etRefOld      = ERL_REFERENCE_EXT     // 'e'
 ,etRef         = ERL_NEW_REFERENCE_EXT // 'r'
 ,etPort        = ERL_PORT_EXT          // 'f'
 ,etPid         = ERL_PID_EXT           // 'g'
 ,etTuple       = ERL_SMALL_TUPLE_EXT   // 'h'
 ,etTupleLarge  = ERL_LARGE_TUPLE_EXT   // 'i'
 ,etNil         = ERL_NIL_EXT           // 'j'
 ,etString      = ERL_STRING_EXT        // 'k'
 ,etList        = ERL_LIST_EXT          // 'l'
 ,etBinary      = ERL_BINARY_EXT        // 'm'
 ,etBignum      = ERL_SMALL_BIG_EXT     // 'n'
 ,etBignumLarge = ERL_LARGE_BIG_EXT     // 'o'
 ,etFun         = ERL_NEW_FUN_EXT       // 'p'
 ,etFunOld      = ERL_FUN_EXT           // 'u'
 ,etNewCache    = ERL_NEW_CACHE         // 'N' /* c nodes don't know these two */
 ,etAtomCached  = ERL_CACHED_ATOM       // 'C'
} erl_type_t;

/* Exports */
ERL_NIF_TERM error(ErlNifEnv* env, const char *fmt, ...);

// Decoders
int decode_command_call_into_process(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[], process_t **ptr);
void ei_list_to_string(ErlNifEnv *env, ERL_NIF_TERM list, char *string);
char *ei_arg_list_to_string(ErlNifEnv *env, ERL_NIF_TERM list, int *arg_size);

#endif
