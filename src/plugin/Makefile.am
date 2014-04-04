AM_CPPFLAGS = -I$(top_srcdir)/src
AM_LDFLAGS = -module -export-symbols-regex '^_plugin_(init|shutdown)'

plugindir = $(libdir)/mjs

plugin_LTLIBRARIES = dummy.la f12automaat.la httpcontroller.la

dummy_la_SOURCES = dummy/dummy.c
dummy_la_LDFLAGS = $(AM_LDFLAGS)

f12automaat_la_SOURCES = f12automaat/f12automaat.c
f12automaat_la_LDFLAGS = $(AM_LDFLAGS)

httpcontroller_la_SOURCES = \
	http_controller/http_controller.c\
	http_controller/json.c
httpcontroller_la_CFLAGS = $(MICROHTTPD_CFLAGS)
httpcontroller_la_LDFLAGS = $(MICROHTTPD_LIBS) $(AM_LDFLAGS)
