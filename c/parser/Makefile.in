# Defines
OUT=test
LIB_SOURCE=honeycomb.l honeycomb.y
LIB_OBJ=y.tab.o lex.yy.o support.o
CC=cc
GCC=g++
CPPFLAGS=-g -O2 -Wall -I ./ -I ../
YACCFLAGS=-d

default: all

all: test_hc_config

clean:
	rm -f *.o y.tab.h

lex:
	lex honeycomb.l

y.tab.h: lex honeycomb.y
	yacc $(YACCFLAGS) honeycomb.y

test_hc_config: honeycomb.c y.tab.h test_hc_config.o $(LIB_OBJ)
	$(GCC) $(CPPFLAGS) -o $(OUT) test_hc_config.cpp $(LIB_OBJ) -ll