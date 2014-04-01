dnl Process this file with autoconf to produce a configure script.
AC_INIT([mjs],[4.1])
LT_INIT[dlopen]

AC_PREREQ([2.69])

AM_INIT_AUTOMAKE([-Wall -Wno-extra-portability -Werror subdir-objects foreign])
AC_LANG([C])
AC_PROG_CC
LT_INIT

# This macro is defined in check.m4 and tests if check.h and
# libcheck.a are installed in your system. It sets CHECK_CFLAGS and
# CHECK_LIBS accordingly.  
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

AC_CONFIG_MACRO_DIR([m4])

AC_PREFIX_DEFAULT(/usr)

AC_HEADER_STDC
AC_CHECK_HEADER([ncurses.h], [], AC_MSG_ERROR([Couldn't find ncurses headers.]))
AC_CHECK_HEADER([panel.h], [], AC_MSG_ERROR([Couldn't find panel headers.]))
AC_CHECK_HEADER([xine.h], [], AC_MSG_ERROR([Couldn't find libxine headers.]))
AC_CHECK_HEADER([pthread.h], [], AC_MSG_ERROR([Couldn't find pthread headers.]))
AC_CHECK_PROGS(apreq2config, apreq2-config, true)

if test "$apreq2config" = "apreq2-config"; then
  apreq_includes=`apreq2-config --includes`
  AC_MSG_NOTICE([Using $apreq_includes for apreq2 include directives])
  AC_SUBST(apreq_includes)
else
  AC_MSG_ERROR([Can not find apreq2-config in $apreq2config path, please install the apreq2 lib and headers])
fi

AC_CHECK_LIB(xine, xine_init, , AC_MSG_ERROR([Couldn't find xine library]), -lxine )
AC_CHECK_LIB(ncursesw, wadd_wch, , AC_MSG_ERROR([Couldn't find ncursesw library]), -lncursesw -ldl)
AC_CHECK_LIB(panelw, refresh, , AC_MSG_ERROR([Couldn't find panel library]), -lpanel)
AC_CHECK_LIB(dl, dlclose, , AC_MSG_ERROR([Couldn't find dlfcn library]), -ldl)
AC_CHECK_LIB(apreq2, apreq_encode, , AC_MSG_ERROR([Couldn't find apreq2 library]), `apreq2-config --link-ld`)

# Checks for library functions.
AC_FUNC_MALLOC

AC_CONFIG_FILES(Makefile src/Makefile tools/Makefile tests/Makefile)
AC_OUTPUT
