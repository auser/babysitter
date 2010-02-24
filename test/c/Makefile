SHELL = /bin/sh

 
top_srcdir = ../..
srcdir = .
prefix = /usr/local
exec_prefix = ${prefix}
bindir = $(exec_prefix)/bin
infodir = $(prefix)/info
libdir = $(prefix)/lib/gnudl
mandir = $(prefix)/man/man1
CFLAGS = $(CPPFLAGS) -I../../c/ -g -O2 -I/include
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
CPPFLAGS =  -I /opt/erlang/lib/erlang/lib/erl_interface-3.6.3/include -Wall -fPIC -I./
LIBS = -lstdc++ 
TEST_SOURCES := $(wildcard check_*.cpp)
TEST_OBJECTS := $(patsubst %.cpp,%.o,$(TEST_SOURCES))
UNDER_TEST := $(patsubst check_%.cpp,%.o,$(TEST_SOURCES))
PACKAGE_NAME = babysitter
PACKAGE_VERSION = 0.1

INTERMEDIATES = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
COMPILED_Os = $(wildcard ../../c/*.o)
OUTPUTFILES := $(INTERMEDIATES)

DEFS = -DHAVE_CONFIG_H
 
ERLDIR = /opt/erlang/lib/erlang
 
all: $(OUTPUTFILES)
 
debug:
	$(MAKE) DEBUG=-DDEBUG
 
.SUFFIXES: .o .cpp .h
  
test_%.o: test_%.c %.o
	$(CC) $(CFLAGS) $(DEFS) $(DEBUG) -c $<

.cpp.o:
	$(CC) $(CFLAGS) $(DEFS) $(DEBUG) -o $@ $(COMPILED_Os) $< $(INC_LIBS)

clean:
	rm -rf *.o *.a $(OUTPUTFILES)
 
install:
	install -d $(ERLDIR)/lib/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/priv
	cp -r `pwd`/../priv $(ERLDIR)/lib/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/
 
package:
	install -d `pwd`/../.pkgtmp/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/priv
	cp -r `pwd`/../priv/* `pwd`/../.pkgtmp/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/priv
