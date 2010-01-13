#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# Initialization
AC_PREREQ([2.64])
AC_INIT(ei++, 0.1, http://github.com/auser/babysitter/issues)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Check the sources
AC_CONFIG_SRCDIR([src/babysitter.cpp])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h string.h sys/time.h unistd.h])
AC_CHECK_HEADERS([sys/param.h])

# Erlang stuff
 
# There is a bug in the Erlang support in autoconf versions after 2.61. So, in
# order to provide a consistent experience, this is a workaround. The bug has
# been fixed, but a new version has not yet been released.
m4_define([AC_LANG(Erlang)],
[ac_ext=erl
 ac_compile='$ERLC $ERLCFLAGS -b beam conftest.$ac_ext >&AS_MESSAGE_LOG_FD'
 ac_link='$ERLC $ERLCFLAGS -b beam conftest.$ac_ext >&AS_MESSAGE_LOG_FD; echo "#!/bin/sh" > conftest$ac_exeext; echo "\"$ERL\" -s conftest -s init stop -noshell" >> conftest$ac_exeext; chmod +x conftest$ac_exeext'
])
 
AC_ERLANG_SUBST_ROOT_DIR
AC_ERLANG_SUBST_INSTALL_LIB_DIR
AC_ERLANG_CHECK_LIB(erl_interface, [], [AC_MSG_FAILURE(cannot find erl_interface, [1])])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_REALLOC
AC_FUNC_CHOWN
AC_FUNC_FORK
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_UID_T
AC_CHECK_FUNCS([gettimeofday memset alarm atexit mkdir regcomp select strchr strdup strerror])

AC_OUTPUT(Makefile src/Makefile)