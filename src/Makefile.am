SUBDIRS = plugin

AM_CPPFLAGS = -D_GNU_SOURCE -D_LARGEFILE64_SOURCE
AM_CFLAGS = -std=c11 -Wall -Werror
AM_LDFLAGS = -static -export-dynamic

lib_LTLIBRARIES = libmjs.la

libmjs_la_SOURCES = \
	defs.h \
	log.h \
	log.c \
	utils.h \
	utils.c \
	config/config.h \
	config/config.c \
	controller/controller.h \
	controller/controller.c \
	engine/engine.h \
	engine/engine.c \
	songdata/songdata.h \
	songdata/songdata.c \
	songdata/dirstack.h \
	songdata/dirstack.c \
	songdata/disk_songdata.h \
	songdata/disk_songdata.c \
	gui/gui.h \
	gui/gui.c \
	gui/window.h \
	gui/window.c \
	gui/filelist.c \
	gui/playlist.c \
	gui/info.c \
	gui/playback.c \
	gui/bar.c \
	plugin/plugin.h \
	plugin/plugin.c
libmjs_la_CFLAGS = $(AM_CFLAGS) $(XINE_CFLAGS) $(CURSES_CFLAGS) $(APREQ2_CFLAGS)
libmjs_la_LDFLAGS = $(AM_LDFLAGS) $(XINE_LIBS) $(CURSES_LIBS) $(APREQ2_LIBS)

bin_PROGRAMS=mjs4

mjs4_SOURCES = \
	mjs.h \
	mjs.c
mjs4_CFLAGS = $(AM_CFLAGS)
mjs4_LDFLAGS = $(AM_LDFLAGS)
mjs4_LDADD=libmjs.la

