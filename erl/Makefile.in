app = babysitter
appdir = $(ERLANG_INSTALL_LIB_DIR)/$(app)-$(VERSION)
 
erl_files = $(wildcard *.erl)
hrl_files = $(wildcard *.hrl)
beam_files = $(erl_files:%.erl=../ebin/%.beam)
ebindir = $(appdir)/ebin
ebin_DATA = $(beam_files)
 
driverdir = $(appdir)/priv/lib
erl_root_dir = @ERLANG_ROOT_DIR@
erl_ei_dir = @ERLANG_LIB_DIR_erl_interface@

babysitter_SOURCES = babysitter.cpp babysitter.h ei++.cpp ei++.h exec.cpp help.h privilege.cpp privilege.h
babysitter_CFLAGS  = -DHAVE_CONFIG_H -I ./ -O -Wall -I$(erl_ei_dir)/include -I$(erl_root_dir)/usr/include
babysitter_LDFLAGS = -fPIC -shared -module -avoid-version -L$(erl_ei_dir)/lib -lerl_interface -lei -lexpat -lcap
 
CLEANFILES = $(ebin_DATA)
EXTRA_DIST = $(erl_files) $(hrl_files)
 
ERLCFLAGS = @ERLCFLAGS@ +warn_obsolete_guard +warn_unused_import \
	+warn_shadow_vars +warn_export_vars +debug_info
 
../ebin/%.beam: %.erl
	$(ERLC) $(ERLCFLAGS) -b beam -o ../ebin $<