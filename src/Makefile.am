bin_PROGRAMS = mjs4
mjs4_SOURCES = log.c mjs.c

# set the include path found by configure
INCLUDES = $(all_includes)

# the library search path.
mjs4_LDFLAGS = $(all_libraries)
SUBDIRS = config controller engine gui serial songdata
mjs4_LDADD = $(top_builddir)/src/serial/libserial.a \
	$(top_builddir)/src/config/libconfig.a $(top_builddir)/src/controller/libcontroller.a \
	$(top_builddir)/src/engine/libengine.a $(top_builddir)/src/gui/libgui.a \
	$(top_builddir)/src/songdata/libsongdata.a
noinst_HEADERS = defs.h log.h mjs.h
