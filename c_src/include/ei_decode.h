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
void nif_list_to_string(ErlNifEnv *env, ERL_NIF_TERM list, char *string);
char *nif_arg_list_to_string(ErlNifEnv *env, ERL_NIF_TERM list, int *arg_size);

// Ei
enum BabysitterActionT {BS_BUNDLE,BS_MOUNT, BS_RUN, BS_UNMOUNT,BS_CLEANUP};
enum BabysitterActionT ei_decode_command_call_into_process(char *buf, process_t **ptr);
int decode_atom_index(char* buf, int *index, const char* cmds[]);

// Ei responses
int ei_pid_ok(int fd, int transId, pid_t pid);
int ei_error(int fd, int transId, const char* fmt, ...);
int ei_ok(int fd, int transId, const char* fmt, ...);

int ei_read(int fd, char** buf);
int read_cmd(int fd, byte **buf, int *size);
int write_cmd(int fd, ei_x_buff *buff);
int read_exact(int fd, byte *buf, int len);
int write_exact(int fd, byte *buf, int len);

#endif
