SHELL = /bin/sh

 
top_srcdir = ..
srcdir = .
prefix = /usr/local
exec_prefix = ${prefix}
bindir = $(exec_prefix)/bin
infodir = $(prefix)/info
libdir = $(prefix)/lib/gnudl
mandir = $(prefix)/man/man1
CFLAGS = $(CPPFLAGS) -g -O2 -I/usr/local/include/include

ifeq ($(shell uname),Linux)
	ARCH = linux
	LDFLAGS = $(LDFLAGS_COMMON) -shared
else
	ARCH = macosx
	LDFLAGS = $(LDFLAGS_COMMON) -dynamic -bundle -undefined suppress -flat_namespace
endif

# Defines
CC = gcc
CPPFLAGS =  -I /opt/erlang/lib/erlang/erts-5.7.4/include -I /opt/erlang/lib/erlang/lib/erl_interface-3.6.4/include -Wall -fPIC -I./
LIBS = 
TEST_SOURCES := $(wildcard check_*.cpp)
TEST_OBJECTS := $(patsubst %.cpp,%.o,$(TEST_SOURCES))
UNDER_TEST := $(patsubst check_%.cpp,%.o,$(TEST_SOURCES))
PACKAGE_NAME = babysitter
PACKAGE_VERSION = 0.1
 
OUTPUTFILES := $(patsubst %.cpp, %.o, $(wildcard *.cpp))
 
DEFS = -DHAVE_CONFIG_H
 
ERLDIR = /opt/erlang/lib/erlang
 
all: $(OUTPUTFILES)
 
debug:
	$(MAKE) DEBUG=-DDEBUG
 
.SUFFIXES: .o .cpp .h
  
test_suite: suite.o $(TEST_OBJECTS) $(UNDER_TEST)
	$(CC) $(CFLAGS) $(DEFS) $(DEBUG) -lcheck -o $@ $^ /opt/erlang/lib/erlang/lib/erl_interface-3.6.4/lib/libei.a	
 
test: test_suite
	$(MAKE) TEST=-DTEST
	./test_suite
 
test-dbg: test_suite
	$(MAKE) TEST=-DTEST
	gdb test_suite
 
test_%.o: test_%.c %.o
	$(CC) $(CFLAGS) $(DEFS) $(DEBUG) -c $<
 
.cpp.o:
	$(CC) $(CFLAGS) $(DEFS) $(DEBUG) -lerl_interface -lei -lelf -lcap -c $<
 
clean:
	rm -rf *.o *.a $(OUTPUTFILES)
 
install:
	install -d $(ERLDIR)/lib/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/priv
	cp -r `pwd`/../priv $(ERLDIR)/lib/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/
 
package:
	install -d `pwd`/../.pkgtmp/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/priv
	cp -r `pwd`/../priv/* `pwd`/../.pkgtmp/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/priv
