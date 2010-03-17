PACKAGE_NAME = babysitter
PACKAGE_VERSION = 0.1
RELDIR = releases/$(PACKAGE_NAME)-$(PACKAGE_VERSION)
 
ifeq ($(shell uname),Linux)
	ARCH = linux
else
	ARCH = macosx
endif

.SUFFIXES: .pc .cpp .c .o

CC=gcc
CXX=g++

MAKEMAKE=src/mm
MYCFLAGS=-DGNU_READLINE -DDEBUG_PRT -g3 -Wall -I./build/readline/include -I./src/utils -I./src/erl

SRCS=$(wildcard src/*.cpp) $(wildcard src/utils/*.cpp) $(wildcard src/erl/*.cpp)
#// babysitter_utils.cpp comb_process.cpp bee.o test.cpp
OBJS=$(patsubst %.cpp, %.o, $(SRCS))
#//babysitter_utils.o comb_process.o bee.o test.o
EXES=$(patsubst src/bin/%.cpp.o, %.o, $(wildcard src/bin/*.cpp))

# For generating makefile dependencies..
SHELL=/bin/sh

CPPFLAGS=$(MYCFLAGS) $(OS_DEFINES)
CFLAGS=$(MYCFLAGS) $(OS_DEFINES)

ALLLDFLAGS= $(LDFLAGS)

COMMONLIBS=-Lbuild/readline/lib/ -lstdc++ -lm -lreadline -lhistory
LIBS=$(COMMONLIBS)

all: prepare_all build_all

prepare_all: $(MAKEMAKE) $(EXES)
	(echo 0.1 > VERSION)

build_all: $(MAKEMAKE) $(EXES)

$(MAKEMAKE):
	@(rm -f $(MAKEMAKE))
	$(CXX) -M  $(INCLUDE) $(CPPFLAGS) $(SRCS) > $(MAKEMAKE)

$(EXES): $(OBJS) clean_bins
	@echo "Creating a executable $(subst src/,,$(subst .cpp,,$@))"
	$(CXX) -c -o $*.o $(INCLUDE) $(CPPFLAGS) -I./src $@

clean_bins:
	@(rm -rf bin/*)
	
bin/*.cpp.o: $(EXES)
	$(CXX) -o $(subst src/,,$(subst .cpp,,$@)) $(subst .cpp,.o,$@) $(OBJS) $(ALLLDFLAGS) $(LIBS)

.cpp.o: $(SRCS) $(HDR)
	$(CXX) -c -o $*.o $(INCLUDE) $(CPPFLAGS) $*.cpp

.c.o: $(SRCS) $(HDR)
	$(CC) -c $(INCLUDE) $(CFLAGS) $*.c

clean:
	rm -rf *.o src/*.o $(EXE) $(MAKEMAKE) test/*.o run_tests

clean_deps:
	rm -rf  build/*

include Tests.makefile

# all:
# 	(echo 0.1 > VERSION)	
# 	(cd c;$(MAKE))
# 	(cd erl;$(MAKE); $(MAKE) boot)
# 
# babysitter:
# 	(cd c;$(MAKE) babysitter)
# 
# clean:
# 	(cd c;$(MAKE) clean)
# 	(cd erl;$(MAKE) clean)
#  
# test:
# 	(cd c; $(MAKE) test)
# 	(cd erl; $(MAKE) test)
#  
# install: all
# 	(cd c; $(MAKE) install)
# 	(cd erl; $(MAKE) install)
#  
# debug:
# 	(cd c;$(MAKE) debug)
# 	(cd erl;$(MAKE) debug)
# 	
# test_deploy: clean
# 	@(rsync -va --exclude=.git --exclude=*.o . vm:~/bs)
