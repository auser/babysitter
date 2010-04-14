
#include "ei_decode.h"
#include "process_manager.h"

/**
* Decode the arguments into a new_process
*
* @params
* {Cmd::string(), [Option]}
*     Option = {env, Strings} | {cd, Dir} | {do_before, Cmd} | {do_after, Cmd} | {nice, int()}
**/
int decode_command_call_into_process(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[], process_t **ptr)
{
  // Instantiate a new process
  if (pm_new_process(ptr))
    return -1;
  
  process_t *process = *ptr;
  const ERL_NIF_TERM* big_tuple;
  int arity = 2;
  // Get the outer tuple
  if(!enif_get_tuple(env, argv[0], &arity, &big_tuple)) return -1;
  
  // The first command is a string
  char command[MAX_BUFFER_SZ], key[MAX_BUFFER_SZ], value[MAX_BUFFER_SZ];
  memset(&command, '\0', sizeof(command));
  
  // Get the command
  if (enif_get_string(env, big_tuple[0], command, sizeof(command), ERL_NIF_LATIN1) < 0) return -1;
  pm_malloc_and_set_attribute(&process->command, command);
  
  // The second element of the tuple is a list of options
  const ERL_NIF_TERM* tuple;
  ERL_NIF_TERM head, tail, list = big_tuple[1];
  
  // int enif_get_tuple(ErlNifEnv* env, ERL_NIF_TERM term, int* arity, const ERL_NIF_TERM** array)
  while(enif_get_list_cell(env, list, &head, &tail)) {
    // Get the tuple
    if(!enif_get_tuple(env, head, &arity, &tuple)) return -1;
    // First element is an atom
    if (!enif_get_atom(env, tuple[0], key, sizeof(key))) return -2;
    if (enif_get_string(env, tuple[1], value, sizeof(value), ERL_NIF_LATIN1) < 0) return -3;
    if (!strcmp(key, "do_before")) {
      // Do before
      pm_malloc_and_set_attribute(&process->before, value);
    } else if (!strcmp(key, "do_after")) {
      pm_malloc_and_set_attribute(&process->after, value);
    } else if (!strcmp(key, "cd")) {
      pm_malloc_and_set_attribute(&process->cd, value);
    } else if (!strcmp(key, "env")) {
      pm_add_env(&process, value);
    } else if (!strcmp(key, "nice")) {
      process->nice = atoi(value);
    }
    list = tail;
  }
  return 0;
}

int ei_read(int fd, char** bufr)
{
  int size = MAX_BUFFER_SZ;
  
  char *buf;
  if ((buf = (char *) calloc(sizeof(char*), size)) == NULL)  return -1;
  
  int ret = read_cmd(fd, &buf, &size);
  
  if (ret > 0) *bufr = buf;
  return ret;
}
/**
* Translate ei buffer into a process_t object
* returns a babysitter_action_t object
**/
const char* babysitter_action_strings[] = {"bundle", "mount", "run", "unmount", "cleanup", NULL};
enum BabysitterActionT ei_decode_command_call_into_process(char *buf, process_t **ptr)
{
  int err_code = -1;
  // Instantiate a new process
  if (pm_new_process(ptr)) return err_code--;
  
  int   arity, index, version, size;
  int i = 0, tuple_size, type;
  long  transId;
    
  // Reset the index, so that ei functions can decode terms from the 
  // beginning of the buffer
  index = 0;

  /* Ensure that we are receiving the binary term by reading and 
   * stripping the version byte */
  if (ei_decode_version(buf, &index, &version) < 0) return err_code--;
  // Decode the tuple header and make sure that the arity is 2
  // as the tuple spec requires it to contain a tuple: {TransId, {Cmd::atom(), Arg1, Arg2, ...}}
  if (ei_decode_tuple_header(buf, &index, &arity) < 0) return err_code--;; // decode the tuple and capture the arity
  if (ei_decode_long(buf, &index, &transId) < 0) return err_code--;; // Get the transId
  if ((ei_decode_tuple_header(buf, &index, &arity)) < 0) return err_code--;; 
  
  process_t *process = *ptr;
  process->transId = transId;
  
  // Get the outer tuple
  // The first command is an atom
  // {Cmd::atom(), Command::string(), Options::list()}
  ei_get_type(buf, &index, &type, &size); 
  char *action = NULL; if ((action = (char*) calloc(sizeof(char*), size + 1)) == NULL) return err_code--;
  // Get the command
  if (ei_decode_atom(buf, &index, action)) return err_code--;
  int ret = -1;
  if ((int)(ret = (enum BabysitterActionT)string_index(babysitter_action_strings, action)) < 0) return err_code--;
  
  // Get the next string
  ei_get_type(buf, &index, &type, &size); 
  char *command = NULL; if ((command = (char*) calloc(sizeof(char*), size + 1)) == NULL) return err_code--;
  
  // Get the command  
  if (ei_decode_string(buf, &index, command) < 0) return err_code--;
  pm_malloc_and_set_attribute(&process->command, command);
  
  // The second element of the tuple is a list of options
  if (ei_decode_list_header(buf, &index, &size) < 0) return err_code--;
  
  enum OptionT            { CD,   ENV,   NICE,   DO_BEFORE,   DO_AFTER } opt;
  const char* options[] = {"cd", "env", "nice", "do_before", "do_after", NULL};
  
  for (i = 0; i < size; i++) {
    // Decode the tuple of the form {atom, string()|int()};
    if (ei_decode_tuple_header(buf, &index, &tuple_size) < 0) return err_code--;
    
    if ((int)(opt = (enum OptionT)decode_atom_index(buf, &index, options)) < 0) return err_code--;
    
    switch (opt) {
      case CD:
      case ENV:
      case DO_BEFORE:
      case DO_AFTER: {
        ei_get_type(buf, &index, &type, &size); 
        char *value = NULL;
        if ((value = (char*) calloc(sizeof(char*), size + 1)) == NULL) return err_code--;
        
        if (ei_decode_string(buf, &index, value) < 0) {
          fprintf(stderr, "ei_decode_string error: %d\n", errno);
          free(value);
          return -9;
        }
        if (opt == CD)
          pm_malloc_and_set_attribute(&process->cd, value);
        else if (opt == ENV)
          pm_add_env(&process, value);
        else if (opt == DO_BEFORE)
          pm_malloc_and_set_attribute(&process->before, value);
        else if (opt == DO_AFTER)
          pm_malloc_and_set_attribute(&process->after, value);
        
        free(value);
      }
      break;
      case NICE: {
        long lval;
        ei_decode_long(buf, &index, &lval);
        process->nice = lval;
      }
      break;
      default:
        return err_code--;
      break;
    }
  }
  
  *ptr = process;
  return 0;
}


//---- Decoders ----//
char read_buf[BUFFER_SZ];
int  read_index = 0;

void nif_list_to_string(ErlNifEnv *env, ERL_NIF_TERM list, char *string)
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

char *nif_arg_list_to_string(ErlNifEnv *env, ERL_NIF_TERM list, int *arg_size)
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

  length = atoi(str_length)+1;
  args = (char *)calloc(length, sizeof(char));

  nif_list_to_string(env, list, args);
  *arg_size = length;

  return args;
};

/**
* ok
**/
ERL_NIF_TERM ok(ErlNifEnv* env, const char* atom, const char *fmt, ...)
{
  char str[MAXATOMLEN];
  va_list vargs;
  va_start (vargs, fmt);
  vsnprintf(str, sizeof(str), fmt, vargs);
  va_end   (vargs);
  
  return enif_make_tuple2(env, enif_make_atom(env,atom), enif_make_atom(env, str));
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


/**
* Data marshalling functions
**/
int ei_write_atom(int fd, int transId, const char* first, const char* fmt, ...)
{
  ei_x_buff result;
  if (ei_x_new_with_version(&result) || ei_x_encode_tuple_header(&result, 2)) return -1;
  if (ei_x_encode_long(&result, transId)) return -2;
  if (ei_x_encode_atom(&result, first) ) return -3;
  // Encode string
  char str[MAXATOMLEN];
  va_list vargs;
  va_start (vargs, fmt);
  vsnprintf(str, sizeof(str), fmt, vargs);
  va_end   (vargs);
  
  if (ei_x_encode_string_len(&result, str, strlen(str))) return -4;
  
  write_cmd(fd, &result);
  ei_x_free(&result);
  return 0;
}

int ei_pid_ok(int fd, int transId, pid_t pid)
{
  ei_x_buff result;
  if (ei_x_new_with_version(&result) || ei_x_encode_tuple_header(&result, 2)) return -1;
  if (ei_x_encode_long(&result, transId)) return -2;
  if (ei_x_encode_tuple_header(&result, 2)) return -3;
  if (ei_x_encode_atom(&result, "ok") ) return -4;
  // Encode pid
  if (ei_x_encode_long(&result, (int)pid)) return -5;
  
  if (write_cmd(fd, &result) < 0) return -5;
  ei_x_free(&result);
  return 0;
}
int ei_ok(int fd, int transId, const char* fmt, ...)
{  
  va_list vargs;
  return ei_write_atom(fd, transId, "ok", fmt, vargs);
}
int ei_error(int fd, int transId, const char* fmt, ...){
  va_list vargs;
  return ei_write_atom(fd, transId, "error", fmt, vargs);
}

int decode_atom_index(char* buf, int *index, const char* cmds[])
{
  int type, size;
  ei_get_type(buf, index, &type, &size); 
  char *atom_name = NULL;
  if ((atom_name = (char*) realloc(atom_name, size + 1)) == NULL) return -1;
  
  if (ei_decode_atom(buf, index, atom_name)) return -1;
  
  int ret = string_index(cmds, atom_name);
  free(atom_name);
  return ret;
}

/**
* Data i/o
**/
int read_cmd(int fd, char **buf, int *size)
{
  int len;
  int header_len = 0;
  if ((header_len = read_exact(fd, *buf, 2)) != 2) return -1;
  
  len = (*buf[0] << 8) | (*buf)[1];

  if (len > *size) {
    byte* tmp = (byte *) realloc(*buf, len);
    if (tmp == NULL)
      return -1;
    else
      *buf = tmp;
    *size = len;
  }
  return read_exact(fd, *buf, len);
}

int write_cmd(int fd, ei_x_buff *buff)
{
  byte li;

  li = (buff->index >> 8) & 0xff;
  write_exact(fd, &li, 1);
  li = buff->index & 0xff;
  write_exact(fd, &li, 1);
  return write_exact(fd, buff->buff, buff->index);
}

int read_exact(int fd, byte *buf, int len)
{
  int i, got=0;

  do {
    if ((i = read(fd, buf+got, len-got)) <= 0)
      return i;
    got += i;
  } while (got<len);

  return len;
}

int write_exact(int fd, byte *buf, int len)
{
  int i, wrote = 0;

  do {
    if ((i = write(fd, buf+wrote, len-wrote)) <= 0)
      return i;
    wrote += i;
  } while (wrote<len);

  return len;
}
