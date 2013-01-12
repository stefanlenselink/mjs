AM_CPPFLAGS = -I$(top_srcdir)/src $(all_includes)
METASOURCES = AUTO
noinst_HEADERS = plugin.h
noinst_LIBRARIES = libplugin.a
libplugin_a_SOURCES = plugin.c


plugindir = $(libdir)/mjs
plugin_LTLIBRARIES = dummy.la f12automaat.la
dummy_la_LDFLAGS = -module -export-symbols-regex '^_plugin_(init|shutdown)'
f12automaat_la_LDFLAGS = -module -export-symbols-regex '^_plugin_(init|shutdown)'
