SHELL = /bin/sh
 
top_srcdir = ..
srcdir = .
prefix = /usr/local
exec_prefix = ${prefix}
bindir = $(exec_prefix)/bin
infodir = $(prefix)/info
libdir = $(prefix)/lib/gnudl
mandir = $(prefix)/man/man1
ERL_DIR =$(shell erl -noshell -eval 'io:format("~s~n", [code:root_dir()])' -s erlang halt)
EI_DIR = $(shell erl -noshell -eval 'io:format("~s~n", [code:lib_dir("erl_interface")])' -s erlang halt)
ERL_ARCH=$(shell erl -noinput +B -eval 'io:format("~B",[8 * erlang:system_info(wordsize)]),halt(0).')

# Set this to '@' for silence
SILENCE = @

#---- Outputs ----#
OUTPUT_DIR = $(top_srcdir)/priv
OUTPUT     = $(OUTPUT_DIR)/bin/babysitter

#CFLAGS are set to override malloc and free to get memory leak detection in C programs
CFLAGS = -O2 -g3 -I $(EI_DIR)/include -I $(ERL_DIR)/usr/include -fPIC
CPPFLAGS += -I $(EI_DIR)/include -I $(ERL_DIR)/usr/include

#CFLAGS are set to override malloc and free to get memory leak detection in C programs
CPPFLAGS += -O3 -fPIC
CPPFLAGS += -Wall
CPPFLAGS += -Werror
CPPFLAGS += -g3

SRC_DIRS = \
	.

INCLUDE_DIRS =\
	.

EXECUTABLE_DIRS =\
	./bin
	
TARGET_LIB_DIRS = \
	./lib	
	
TEST_DIRS = \
	./tests

#Flags to pass to ld
LDFLAGS_COMMON=-L$(EI_DIR)/lib -fno-common -Wall -pthread
ifeq ($(shell uname),Linux)
	ARCH=linux
	LDFLAGS=$(LDFLAGS_COMMON)
	LDBUNDLE_FLAGS=$(LDFLAGS) -shared
else
	# -dynamic -undefined suppress -flat_namespace
	ARCH=macosx
	LDFLAGS=$(LDFLAGS_COMMON) -flat_namespace
	LDBUNDLE_FLAGS=$(LDFLAGS) -bundle
endif

ifeq ($ERL_ARCH,32)
	CPPFLAGS += -m32
endif

LD_LIBRARIES += -lerl_interface -lei

# DONT TOUCH BELOW HERE!
#
# SERIOUSLY
#
#
# I'M NOT KIDDING
#
# IF YOU DO
# I CAN'T HELP YOU
#
# PROCEED AT YOUR OWN RISK 

#Helper Functions
get_src_from_dir  = $(wildcard $1/*.cpp) $(wildcard $1/*.c)
get_dirs_from_dirspec  = $(wildcard $1)
get_src_from_dir_list = $(foreach dir, $1, $(call get_src_from_dir,$(dir)))				        
src_to_o = $(subst .c,.o, $(subst .cpp,.o,$1))
src_to = $(subst .c,$1, $(subst .cpp,$1,$2))

SRC = $(call get_src_from_dir_list, $(SRC_DIRS))
OBJ = $(call src_to_o,$(SRC))
STUFF_TO_CLEAN += $(OBJ)

EXECUTABLE_SRC = $(call get_src_from_dir_list, $(EXECUTABLE_DIRS))
EXECUTABLE_OBJ = $(call src_to_o, $(EXECUTABLE_SRC))
STUFF_TO_CLEAN += $(EXECUTABLE_OBJ)

TARGET_LIBS_SRC = $(call get_src_from_dir_list, $(TARGET_LIB_DIRS))
TARGET_LIBS_OBJ = $(call src_to,.so, $(TARGET_LIBS_SRC))
STUFF_TO_CLEAN += $(TARGET_LIBS_OBJ)

ALL_SRC = $(SRC) $(EXECUTABLE_SRC)
ALL_OBJ = $(OBJ) $(EXECUTABLE_OBJ)

# Tests
TEST_SRC	= $(call get_src_from_dir_list,$(TEST_DIRS))
TEST_OBJ	= $(call src_to_o,$(TEST_SRC))
STUFF_TO_CLEAN += $(TEST_OBJ)

INCLUDES_DIRS_EXPANDED = $(call get_dirs_from_dirspec, $(INCLUDE_DIRS))
INCLUDES += $(foreach dir, $(INCLUDES_DIRS_EXPANDED), -I$(dir))

CPPFLAGS +=  $(INCLUDES)
CFLAGS += $(INCLUDES)

# Targets
.PHONY: all
all: $(ALL_OBJ) $(TARGET_LIBS_OBJ) build_tests build_executable

build_tests:
	$(SILENCE)echo "Building c_src tests"
	$(SILENCE)$(CC) $(INCLUDES) -o run_tests process_manager.o pm_helpers.o $(LDFLAGS_COMMON) $(LD_LIBRARIES) $(TEST_SRC)

build_executable:
	$(SILENCE)mkdir -p `dirname $(OUTPUT)`
	$(SILENCE)$(CC) -o $(OUTPUT) $(OBJ) $(LDFLAGS) $(LD_LIBRARIES) $(EXECUTABLE_OBJ)
	
$(TARGET_LIBS_OBJ): $(OBJ)
	$(SILENCE)echo Linking $@
	$(SILENCE)mkdir -p `dirname $(OUTPUT_DIR)/$@`
	$(SILENCE)$(LINK.o) -o $(OUTPUT_DIR)/$@ $^ $(CFLAGS) $(LD_LIBRARIES) $(subst .so,.c,$@)

%.o: %.cpp
	$(SILENCE)echo compiling $(notdir $<)
	$(SILENCE)$(COMPILE.c) $(OUTPUT_OPTION) $<
	
# $(SILENCE)$(COMPILE.c) -M -MF $(subst .o,.d,$@) -MT "$@ $(subst .o,.d,$@)" $<
%.o: %.c
	$(SILENCE)echo compiling $(notdir $<)
	$(SILENCE)$(COMPILE.c) $(OUTPUT_OPTION) $<

.PHONY: clean
clean:
	$(SILENCE)echo House cleaning...
	$(SILENCE)$(RM) $(STUFF_TO_CLEAN)
