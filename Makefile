PACKAGE_NAME = babysitter
PACKAGE_VERSION = 0.1
RELDIR = releases/$(PACKAGE_NAME)-$(PACKAGE_VERSION)
export ROOT_DIR = /Users/alerner/Development/erlang/mine/babysitter
 
ifeq ($(shell uname),Linux)
	ARCH = linux
else
	ARCH = macosx
endif

.PHONY: deps

all:
	make -C c_src
	./rebar compile

docs:
	@mkdir -p docs
	@./build_docs.sh

deps:
	./rebar get-deps

clean:
	rm -rf tests_ebin docs
	./rebar clean
	cd c_src;make clean

test: all
	@mkdir -p tests_ebin
	@cd tests;erl -make
	@erl -noshell -boot start_sasl -I ./include -pa ebin -pa tests_ebin -pa ebin -s test_suite test -s init stop
	@rm -f ebin/test_* ebin/*_tests.erl
 
install: all
	@./rebar install
 