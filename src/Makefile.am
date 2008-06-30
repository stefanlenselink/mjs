bin_PROGRAMS = mjs4
mjs4_SOURCES = config.c engine.c files.c gui.c gui/question.c gui/window.c \
	info.c inputline.c lastfm.c list.c misc.c mjs.c mjs_id3.c mpgcontrol.c \
	playlist.c tokens.c

# set the include path found by configure
INCLUDES = -I$(top_srcdir)/src/gui $(all_includes)

# the library search path.
mjs4_LDFLAGS = $(all_libraries)
SUBDIRS = gui
