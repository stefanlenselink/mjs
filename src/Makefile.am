bin_PROGRAMS = mjs4
mjs4_SOURCES = config.c engine.c files.c gui.c gui/question.c gui/window.c \
	info.c inputline.c lastfm.c list.c misc.c mjs.c mjs4.c mjs_id3.c mpgcontrol.c \
	playlist.c tokens.c

# set the include path found by configure
INCLUDES = -I$(top_srcdir)/src/gui $(all_includes)

# the library search path.
mjs4_LDFLAGS = $(all_libraries)
noinst_HEADERS = colors.h config.h defs.h engine.h extern.h files.h info.h \
	inputline.h lastfm.h list.h misc.h mjs.h mjs_id3.h mpgcontrol.h playlist.h struct.h \
	tokens.h top.h
SUBDIRS = gui
