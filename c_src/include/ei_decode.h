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

/* Exports */
ERL_NIF_TERM ok(ErlNifEnv* env, const char* atom, const char *fmt, ...);
ERL_NIF_TERM error(ErlNifEnv* env, const char *fmt, ...);

// Decoders
int decode_command_call_into_process(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[], process_t **ptr);
void ei_list_to_string(ErlNifEnv *env, ERL_NIF_TERM list, char *string);
char *ei_arg_list_to_string(ErlNifEnv *env, ERL_NIF_TERM list, int *arg_size);

#endif
