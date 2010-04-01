#include "callbacks.h"

// Globals
MapCallbackFuncT              callbacks;

/**
* Register a callback
**/
void register_callback(std::string callback_name, void (*callback_event_function)(void *state))
{
  CallbackListT list;
  if(callbacks.count(callback_name) != 0) {
    // It's already in there!
    MapCallbackFuncT::iterator it = callbacks.find(callback_name);
    list = it->second;
    callbacks.erase(it); // Clear the old one :)
  }
  // Add the callback
  list.push_back(callback_event_function);
  callbacks.insert(std::make_pair(callback_name, list));
}

void run_callbacks(std::string callback_name, void *data)
{  
  if(callbacks.count(callback_name) != 0) {
    // It's already in there!
    MapCallbackFuncT::iterator callbacks_it = callbacks.find(callback_name);
    CallbackListT list = callbacks_it->second;
    
    CallbackListT::iterator it;
    for ( it = list.begin(); it != list.end(); it++ ) {
      callback_event_function f = *it;
      f(data);
    }
  }
  
}