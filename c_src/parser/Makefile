# Defines
OUT=test
LIB_SOURCE=honeycomb.l honeycomb.y
LIB_OBJ=y.tab.o lex.yy.o ../babysitter/hc_support.o
CC=cc
GCC=g++
CPPFLAGS=-g -O2 -Wall -I ./ -I ../babysitter -I ../utils
YACCFLAGS=-d

default: all

all: honeycomb.c y.tab.h $(LIB_OBJ)

clean:
	rm -rf *.o y.tab.h *.dSYM

lex:
	lex honeycomb.l

y.tab.h: lex honeycomb.y 
	yacc $(YACCFLAGS) honeycomb.y

test_hc_config: honeycomb.c y.tab.h hc_support.o $(LIB_OBJ)
	$(GCC) $(CPPFLAGS) -o $(OUT) test_hc_config.cpp $(LIB_OBJ) -ll

test_match: honeycomb.c y.tab.h test_hc_config.o $(LIB_OBJ)
	$(CC) $(CPPFLAGS) -o $(OUT)_parser honeycomb.c lex.yy.o hc_support.o -lfl
