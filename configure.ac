dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(mjs4, 1.0)


AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([subdir-objects])

AC_PROG_CC

AC_PREFIX_DEFAULT(/usr)

AC_CHECK_HEADER([ncurses.h], [], AC_MSG_ERROR([Couldn't find ncurses headers.]))

AC_CHECK_HEADER([panel.h], [], AC_MSG_ERROR([Couldn't find panel headers.]))

AC_CHECK_HEADER([xine.h], [], AC_MSG_ERROR([Couldn't find libxine headers.]))

AC_CHECK_HEADER([mysql/mysql.h], [], AC_MSG_ERROR([Couldn't find libmysqlclient headers.]))

AC_CHECK_LIB(xine, xine_init, , AC_MSG_ERROR([Couldn't find xine library]), -lxine)

AC_CHECK_LIB(ncurses, refresh, , AC_MSG_ERROR([Couldn't find ncurses library]), -lncurses -ldl)

AC_CHECK_LIB(ncursesw, wadd_wch, , AC_MSG_ERROR([Couldn't find ncursesw library]), -lncursesw -ldl)

AC_CHECK_LIB(panel, refresh, , AC_MSG_ERROR([Couldn't find panel library]), -lpanel )

AC_CHECK_LIB(mysqlclient, mysql_init, , AC_MSG_ERROR([Couldn't find mysqlclient library]), -lmysqlclient -lz )
AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT

