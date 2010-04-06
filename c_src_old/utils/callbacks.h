#ifndef CALLBACK_H
#define CALLBACK_H

#include <string>
#include <map>
#include <list>

// Types
typedef void (*callback_event_function)(void *state);
typedef std::list<callback_event_function> CallbackListT;
typedef std::map<std::string, CallbackListT> MapCallbackFuncT;

// Functions
void register_callback(std::string callback_name, void (*callback_event_function)(void *state));
void run_callbacks(std::string callback_name, void *data);

#endif