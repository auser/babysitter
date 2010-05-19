#!/bin/bash

export ERL_LIBS=`cd ..;pwd`;
erl -noshell -eval 'edoc:application(babysitter, [{dir, "docs"}]), init:stop().'