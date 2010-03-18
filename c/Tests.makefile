CMOCKERY_DIR=build/cmockery
LIBMOCKERY=$(CMOCKERY_DIR)/lib/libcmockery.a
INCMOCKERY=$(CMOCKERY_DIR)/include/google

TEST_CFLAGS = $(CFLAGS) -Isrc -Itest -I $(INCMOCKERY)

SRC_OBJ=src/string_utils.o
TEST_SRCS=$(wildcard test/*.c)
TEST_OBJ = $(TEST_SRCS:.c=.o)

$(TEST_OBJ): %.o: %.c
	@$(CC) -c $(TEST_CFLAGS) $< -o $@

$(LIBMOCKERY):
	sh build-test-deps.sh

tests: clean_tests all $(LIBMOCKERY) $(TEST_OBJ)
	@$(LINK.cc) -o run_tests $(TEST_OBJ) $(SRC_OBJ) $(LIBS) build/cmockery/lib/libcmockery.a

clean_tests:
	@(rm -rf test/*.o)