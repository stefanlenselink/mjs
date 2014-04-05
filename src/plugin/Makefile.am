AM_CPPFLAGS = -I$(top_srcdir)/src -D_GNU_SOURCE -D_LARGEFILE_64_SOURCE
AM_CFLAGS = -std=c11 -Wall -Werror
AM_LDFLAGS = -module -export-symbols-regex '^_plugin_(init|shutdown)'

plugindir = $(libdir)/mjs

plugin_LTLIBRARIES = dummy.la f12automaat.la httpcontroller.la

dummy_la_SOURCES = dummy/dummy.c

f12automaat_la_SOURCES = f12automaat/f12automaat.c

httpcontroller_la_SOURCES = \
	http_controller/http_controller.c\
	http_controller/json.c
httpcontroller_la_CFLAGS = $(AM_CFLAGS) $(MICROHTTPD_CFLAGS)
httpcontroller_la_LDFLAGS = $(AM_LDFLAGS) $(MICROHTTPD_LDFLAGS) 
