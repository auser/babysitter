
#include "ei_decode.h"
#include "process_manager.h"

/**
* Decode the arguments into a new_process
*
* @params
* {Cmd::string(), [Option]}
*     Option = {env, Strings} | {cd, Dir} | {kill, Cmd}
**/
int decode_into_process(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[], process_t **ptr)
{
  // Instantiate a new process
  if (pm_new_process(ptr))
    return -1;
  
  char command[MAX_BUFFER_SZ];
  (void)memset(&command, '\0', sizeof(command));
  
  if (enif_get_string(env, argv[0], command, sizeof(command), ERL_NIF_LATIN1) < 0) {
    return error(env, "Unable to decode command");
  }
  process_t *process = *ptr;
  malloc_and_set_attribute(&process->command, command);
  
  printf("process->command: %s\n", process->command);
  return 0;
}

//---- Decoders ----//
char read_buf[BUFFER_SZ];
int  read_index = 0;

void ei_list_to_string(ErlNifEnv *env, ERL_NIF_TERM list, char *string)
{
  ERL_NIF_TERM head, tail;
  int character;

  while(enif_get_list_cell(env, list, &head, &tail)) {
    if(!enif_get_int(env, head, &character)) {
      return;
    }
    
    *string++ = (char)character;
    list = tail;
  };

  *string = '\0';
};

char *ei_arg_list_to_string(ErlNifEnv *env, ERL_NIF_TERM list, int *arg_size)
{
  ERL_NIF_TERM head, tail;
  char str_length[PREFIX_LEN], *args;
  int i, length, character;

  for(i=0; i<PREFIX_LEN; i++) {
    if(enif_get_list_cell(env, list, &head, &tail)) {
      if(!enif_get_int(env, head, &character)) {
        return NULL;
      }
      str_length[i] = (char)character;
      list = tail;
    } else {
      return NULL;
    }
  };

  fprintf(stderr, "here...\n");
  length = atoi(str_length)+1;
  args = (char *)calloc(length, sizeof(char));

  ei_list_to_string(env, list, args);
  *arg_size = length;

  return args;
};


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

