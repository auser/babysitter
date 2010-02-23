SHELL = /bin/sh

 
top_srcdir = ..
srcdir = .
prefix = /usr/local
exec_prefix = ${prefix}
bindir = $(exec_prefix)/bin
infodir = $(prefix)/info
libdir = $(prefix)/lib/gnudl
mandir = $(prefix)/man/man1
CFLAGS = $(CPPFLAGS) -g -O2 -I ./parser
ERL_INTERFACE=${ERLDIR}/lib/${ERL_INTERFACE}
PARSER_LIBS = $(top_srcdir)/c/parser/y.tab.o $(top_srcdir)/c/parser/lex.yy.o
INC_LIBS = -L/usr/local/lib/erlang/lib/erl_interface-3.6.5/lib -lerl_interface -lei -lstdc++ -lfl
EXECUTABLE_DIR = $(top_srcdir)/priv/bin

ifeq ($(shell uname),Linux)
	ARCH=linux
	LDFLAGS=$(LDFLAGS_COMMON) -shared
else
	ARCH=macosx
	LDFLAGS=$(LDFLAGS_COMMON) -dynamic -bundle -undefined suppress -flat_namespace
endif

# Defines
CC = g++
CPPFLAGS =  -I /usr/local/lib/erlang/lib/erl_interface-3.6.5/include -Wall -fPIC -I./
LIBS = -lstdc++ 
TEST_SOURCES := $(wildcard check_*.cpp)
TEST_OBJECTS := $(patsubst %.cpp,%.o,$(TEST_SOURCES))
UNDER_TEST := $(patsubst check_%.cpp,%.o,$(TEST_SOURCES))
PACKAGE_NAME = babysitter
PACKAGE_VERSION = 0.1

INTERMEDIATES = honeycomb.o hc_support.o honeycomb_config.o
#$(patsubst %.cpp, %.o, $(wildcard *.cpp))
OUTPUTFILES := $(INTERMEDIATES) config_parser babysitter babysitter_daemon
#OUTPUTFILES := config_parser honeycomb eipp babysitter 

DEFS = -DHAVE_CONFIG_H
 
ERLDIR = /usr/local/lib/erlang
 
all: prepare honeycomb $(OUTPUTFILES)

prepare:
	mkdir -p $(EXECUTABLE_DIR)

debug:
	$(MAKE) DEBUG=-DDEBUG
 
.SUFFIXES: .o .cpp .h
  
test_suite: suite.o $(TEST_OBJECTS) $(UNDER_TEST)
	$(CC) $(CFLAGS) $(DEFS) $(DEBUG) -lcheck -o $@ $^ /usr/local/lib/erlang/lib/erl_interface-3.6.5/lib/libei.a	
 
test: test_suite
	$(MAKE) TEST=-DTEST
	./test_suite
 
test-dbg: test_suite
	$(MAKE) TEST=-DTEST
	gdb test_suite
 
test_%.o: test_%.c %.o
	$(CC) $(CFLAGS) $(DEFS) $(DEBUG) -c $<

.cpp.o:
	$(CC) $(CFLAGS) $(DEFS) $(DEBUG) -c $<

eipp:
	$(CC) $(CFLAGS) $(DEFS) $(DEBUG) -c ei++.cpp

config_parser:
	@(cd ./parser;$(MAKE)) # make the config parser

honeycomb: config_parser honeycomb.cpp honeycomb.h
	$(CC) $(CFLAGS) $(DEFS) $(DEBUG) -c honeycomb.cpp

honeycomb_config.o: honeycomb
	$(CC) $(CFLAGS) $(DEFS) $(DEBUG) -c honeycomb_config.cpp

babysitter_daemon:
	$(CC) $(CFLAGS) -I/include -o $(EXECUTABLE_DIR)/babysitter_daemon babysitter_daemon.cpp $(INTERMEDIATES) $(PARSER_LIBS) $(LDFLAGS) $(INC_LIBS)

babysitter:
	$(CC) $(CFLAGS) $(DEFS) $(DEBUG) -o $(EXECUTABLE_DIR)/babysitter babysitter.cpp $(INTERMEDIATES) $(PARSER_LIBS)

clean:
	rm -rf *.o *.a $(OUTPUTFILES) $(EXECUTABLE_DIR)/*
	cd ./parser; $(MAKE) clean
 
install:
	install -d $(ERLDIR)/lib/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/priv
	cp -r $(EXECUTABLE_DIR)/* $(ERLDIR)/lib/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/
 
package:
	install -d $(top_srcdir)/.pkgtmp/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/priv
	cp -r $(EXECUTABLE_DIR)/* $(top_srcdir)/../.pkgtmp/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/priv
