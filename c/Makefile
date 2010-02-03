SHELL = /bin/sh

 
top_srcdir = ..
srcdir = .
prefix = /usr/local
exec_prefix = ${prefix}
bindir = $(exec_prefix)/bin
infodir = $(prefix)/info
libdir = $(prefix)/lib/gnudl
mandir = $(prefix)/man/man1
CFLAGS = $(CPPFLAGS) -g -O2 -I/usr/local/include
ERL_INTERFACE=${ERLDIR}/lib/${ERL_INTERFACE}
INC_LIBS = -lerl_interface -lei -lcap -lstdc++ -lelf
EXECUTABLE = ../priv/bin/babysitter

ifeq ($(shell uname),Linux)
	ARCH = linux
	LDFLAGS = $(LDFLAGS_COMMON) -shared
else
	ARCH = macosx
	LDFLAGS = $(LDFLAGS_COMMON) -dynamic -bundle -undefined suppress -flat_namespace
endif

# Defines
CC = g++
CPPFLAGS =  -I /usr/lib/erlang/lib/erl_interface-3.6.2/include -Wall -fPIC -I./
LIBS = -lcap -lelf -lstdc++ 
TEST_SOURCES := $(wildcard check_*.cpp)
TEST_OBJECTS := $(patsubst %.cpp,%.o,$(TEST_SOURCES))
UNDER_TEST := $(patsubst check_%.cpp,%.o,$(TEST_SOURCES))
PACKAGE_NAME = babysitter
PACKAGE_VERSION = 0.1

INTERMEDIATES = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
OUTPUTFILES := $(INTERMEDIATES) worker_bee babysitter
#OUTPUTFILES := config_parser honeycomb eipp babysitter 

DEFS = -DHAVE_CONFIG_H
 
ERLDIR = /usr/lib/erlang
 
all: honeycomb $(OUTPUTFILES)
 
debug:
	$(MAKE) DEBUG=-DDEBUG
 
.SUFFIXES: .o .cpp .h
  
test_suite: suite.o $(TEST_OBJECTS) $(UNDER_TEST)
	$(CC) $(CFLAGS) $(DEFS) $(DEBUG) -lcheck -o $@ $^ /usr/lib/erlang/lib/erl_interface-3.6.2/lib/libei.a	
 
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
	$(CC) $(CFLAGS) $(DEFS) $(DEBUG) -c config_parser.cpp

honeycomb: worker_bee honeycomb.cpp honeycomb.h
	$(CC) $(CFLAGS) $(DEFS) $(DEBUG) -c honeycomb.cpp

worker_bee: worker_bee.cpp worker_bee.h
	$(CC) $(CFLAGS) $(DEFS) $(DEBUG) -c worker_bee.cpp

babysitter:
	mkdir -p `dirname $(EXECUTABLE)`
	$(CC) $(CFLAGS) $(DEFS) $(DEBUG) -o $(EXECUTABLE) $(INTERMEDIATES) $(INC_LIBS)

clean:
	rm -rf *.o *.a $(OUTPUTFILES) $(EXECUTABLE)
 
install:
	install -d $(ERLDIR)/lib/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/priv
	cp -r `pwd`/../priv $(ERLDIR)/lib/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/
 
package:
	install -d `pwd`/../.pkgtmp/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/priv
	cp -r `pwd`/../priv/* `pwd`/../.pkgtmp/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/priv
