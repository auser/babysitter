#ifndef BABYSITTER_H
#define BABYSITTER_H

#include "honeycomb.h"

#ifndef BABYSITTER_VERSION
// TODO: Make this read from the VERSION file at root
#define BABYSITTER_VERSION 0.1
#endif

//---
// Variables
//---
std::deque <PidStatusT> exited_children;

//---
// Functions
//---
void  setup_defaults();
void setup_signal_handlers(struct sigaction sact, struct sigaction sterm, std::deque<PidStatusT> exited_children);

#endif