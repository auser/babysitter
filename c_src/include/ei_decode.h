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
#include "pm_helpers.h"

// Defines
typedef char byte;

/* Exports */
ERL_NIF_TERM ok(ErlNifEnv* env, const char* atom, const char *fmt, ...);
ERL_NIF_TERM error(ErlNifEnv* env, const char *fmt, ...);

// Decoders
int decode_command_call_into_process(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[], process_t **ptr);
void ei_list_to_string(ErlNifEnv *env, ERL_NIF_TERM list, char *string);
char *ei_arg_list_to_string(ErlNifEnv *env, ERL_NIF_TERM list, int *arg_size);

// Ei
int ei_decode_command_call_into_process(char *buf, process_t **ptr);
int decode_atom_index(char* buf, int index, const char* cmds[]);

int ei_error(int fd, const char* fmt, va_list vargs);
int ei_ok(int fd, const char* fmt, va_list vargs);

int read_cmd(byte **buf, int *size, int fd);
int write_cmd(ei_x_buff* x, int fd);
int read_exact(byte *buf, int len, int fd);
int write_exact(byte *buf, int len, int fd);

#endif
