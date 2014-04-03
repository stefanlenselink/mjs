AM_LDFLAGS=-static

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
	gui/bar.c\
	plugin/http_controller/http_controller.c\
	plugin/http_controller/json.c
libmjs_la_CFLAGS=$(apreq_includes) -std=c11 -Wall -Werror -D_GNU_SOURCE -D_LARGEFILE64_SOURCE

bin_PROGRAMS=mjs4
mjs4_SOURCES = \
	mjs.c
mjs4_CFLAGS=$(apreq_includes) -std=c11 -Wall -Werror -D_GNU_SOURCE -D_LARGEFILE64_SOURCE
mjs4_LDADD=libmjs.la
