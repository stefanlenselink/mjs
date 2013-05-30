dnl Process this file with autoconf to produce a configure script.
AC_INIT([mjs], [4.1])
AM_INIT_AUTOMAKE
LT_INIT[dlopen]

AC_PREREQ(2.59)

AM_INIT_AUTOMAKE([subdir-objects foreign])
AC_LANG_C
AC_PROG_CC

AC_PREFIX_DEFAULT(/usr)

AC_CHECK_HEADER([ncurses.h], [], AC_MSG_ERROR([Couldn't find ncurses headers.]))
AC_CHECK_HEADER([panel.h], [], AC_MSG_ERROR([Couldn't find panel headers.]))
AC_CHECK_HEADER([xine.h], [], AC_MSG_ERROR([Couldn't find libxine headers.]))
AC_CHECK_HEADER([pthread.h], [], AC_MSG_ERROR([Couldn't find pthread headers.]))
AC_CHECK_HEADER([dlfcn.h], [], AC_MSG_ERROR([Couldn't find dlfcn headers.]))

AC_CHECK_LIB(xine, xine_init, , AC_ERROR([Couldn't find xine library]), -lxine, )
AC_CHECK_LIB(ncursesw, wadd_wch, , AC_ERROR([Couldn't find ncursesw library]), -lncursesw, -ldl)
AC_CHECK_LIB(panelw, refresh, , AC_ERROR([Couldn't find panel library]), -lpanel, )
AC_CHECK_LIB(microhttpd, MHD_get_version, , AC_ERROR([Couldn't find microhttpd library]), -lmicrohttpd, )
AC_CHECK_LIB(pthread, pthread_create, , AC_ERROR([Couldn't find pthread library]), -lpthread, )
AC_CHECK_LIB(dl, dlclose, , AC_ERROR([Couldn't find dlfcn library]), -ldl, )

AC_CONFIG_FILES(Makefile src/Makefile tools/src/Makefile)
AC_OUTPUT
