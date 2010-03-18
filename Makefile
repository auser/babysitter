PACKAGE_NAME = babysitter
PACKAGE_VERSION = 0.1
RELDIR = releases/$(PACKAGE_NAME)-$(PACKAGE_VERSION)
 
ifeq ($(shell uname),Linux)
	ARCH = linux
else
	ARCH = macosx
endif
 
all:
	(echo 0.1 > VERSION)	
	(cd c_src;$(MAKE))
	(cd erl;$(MAKE); $(MAKE) boot)

c:
	(cd c_src;$(MAKE))

clean:
	(cd c_src;$(MAKE) clean)
	(cd erl;$(MAKE) clean)
 
test:
	(cd c_src; $(MAKE) test)
	(cd erl; $(MAKE) test)
 
install: all
	(cd c_src; $(MAKE) install)
	(cd erl; $(MAKE) install)
 
debug:
	(cd c_src;$(MAKE) debug)
	(cd erl;$(MAKE) debug)
	
