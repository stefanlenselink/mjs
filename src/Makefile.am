lib_LTLIBRARIES = libmjs.la
libmjs_la_SOURCES = \
	engine/engine.c \
	config/config.c \
	songdata/dirstack.c \
	songdata/song.c \
	controller/controller.c \
	songdata/disk_songdata.c \
	gui/gui.c \
	log.c \
	songdata/songdata.c \
	controller/json.c \
	controller/http_controller.c \
	gui/window.c \
	gui/filelist.c \
	gui/playlist.c \
	gui/info.c \
	gui/playback.c \
	gui/bar.c \
	plugin/plugin.c \
	plugin/f12automaat.c \
	mjs.c
libmjs_la_CFLAGS=$(apreq_includes) -Wall -Werror -Wno-unused-result -Wno-unused-but-set-variable -D_GNU_SOURCE

bin_PROGRAMS=mjs4
mjs4_SOURCES = \
	mjs.c
mjs4_CFLAGS=$(apreq_includes) -Wall -Werror -Wno-unused-result -Wno-unused-but-set-variable -D_GNU_SOURCE
mjs4_LDADD=libmjs.la
