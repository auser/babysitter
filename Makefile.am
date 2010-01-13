# Configuration options
TOMAKE_OPTIONS = foreign
SUBDIRS = src man

SUBDIRS = src test
 
clean-hook:
	rm -rf ebin/*
	rm -rf priv/lib/*
 
dist-hook:
	mkdir $(distdir)/ebin
 
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

