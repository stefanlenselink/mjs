AM_LDFLAGS=-no-install

TESTS = check_mjs
bin_PROGRAMS = check_mjs
check_PROGRAMS = check_mjs
check_mjs_SOURCES = check_mjs.c
check_mjs_CFLAGS = @CHECK_CFLAGS@ -I $(top_builddir)/src
check_mjs_LDADD = @CHECK_LIBS@ $(top_builddir)/src/libmjs.la

