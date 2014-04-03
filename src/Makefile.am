SUBDIRS = plugin

AM_CPPFLAGS = -D_GNU_SOURCE -D_LARGEFILE64_SOURCE
AM_CFLAGS = -std=c11

lib_LTLIBRARIES = libmjs.la

libmjs_la_SOURCES = \
	engine/engine.c \
	config/config.c \
	songdata/dirstack.c \
	controller/controller.c \
	songdata/disk_songdata.c \
	gui/gui.c \
	log.c \
	utils.c \
	songdata/songdata.c \
	gui/window.c \
	gui/filelist.c \
	gui/playlist.c \
	gui/info.c \
	gui/playback.c \
	gui/bar.c \
	plugin/plugin.c \
	mjs.c
libmjs_la_CFLAGS = $(XINE_CFLAGS) $(CURSES_CFLAGS) $(APREQ2_CFLAGS)
libmjs_la_LDFLAGS = $(XINE_LIBS) $(CURSES_LIBS) $(APREQ2_LIBS)

bin_PROGRAMS=mjs4

mjs4_SOURCES = \
	mjs.c
mjs4_LDADD=libmjs.la
