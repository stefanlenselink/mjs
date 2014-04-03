dnl Process this file with autoconf to produce a configure script.
AC_INIT([mjs],[4.2])

AC_PREREQ([2.69])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES([Makefile src/Makefile src/plugin/Makefile tools/Makefile tests/Makefile])
AC_CONFIG_HEADERS([config.h])
AC_PREFIX_DEFAULT([/usr])

AM_INIT_AUTOMAKE([-Wall -Wno-extra-portability -Werror subdir-objects foreign])

AC_LANG([C])
AC_PROG_CC
AC_PROG_LIBTOOL

LT_INIT([dlopen])

PKG_CHECK_MODULES([XINE], [libxine])
PKG_CHECK_MODULES([CURSES], [ncursesw panelw])
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

AC_MSG_CHECKING([for APREQ2])
AS_IF([which apreq2-config &> /dev/null],
  [
    APREQ2_CFLAGS=`apreq2-config --includes`
    APREQ2_LIBS=`apreq2-config --link-ld`
    AC_SUBST([APREQ2_CFLAGS])
    AC_SUBST([APREQ2_LIBS])
	AC_MSG_RESULT([yes])
  ],
  [
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([Couldn't find apreq2-config, please install the apreq2 library and headers.])
  ]
)

AC_OUTPUT
