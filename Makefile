CC = gcc
prefix = /usr/local/bin

VERSION = 3.3

SRCS =	mjs.c misc.c info.c config.c playlist.c inputline.c mpgcontrol.c \
	tokens.c window.c files.c list.c
OBJS =	mjs.o misc.o info.o config.o playlist.o inputline.o mpgcontrol.o \
	tokens.o window.o files.o list.o
INCLUDES = -Iinclude
LIBRARY = -L/usr/local/lib
PROFILE = #-pg
LIBS = -lncurses -lpanel 
ARCHFLAGS = -mcpu=pentium
#ARCHFLAGS = -march=pentium2
WARNINGS = -Wbad-function-cast -Wcast-align 
#WARNINGS = -Wall -Wbad-function-cast -Wcast-align -unreachable-code -W -Wshadow
OPTFLAGS = -O2
# Comment this out for debugging
#OPTFLAGS += -g3

CFLAGS = $(OPTFLAGS) $(PROFILE) $(WARNINGS) $(ARCHFLAGS) -DVERSION=\"$(VERSION)\"

# Uncomment these if you use FreeBSD and their weird ncurses port
#CFLAGS += -DFREEBSD_NCURSES_PORT

all: mjs

default: all

run: all
	./mjs

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

mjs: $(OBJS)
	$(CC) $(PROFILE) $(ARCHFLAGS) -o mjs $(OBJS) $(LIBRARY) $(LIBS)

static : $(OBJS)
	@echo "" 
	$(CC) -static $(PROFILE) $(ARCHFLAGS) -o mjs $(OBJS) $(LIBRARY) $(LIBS)

clean:
	rm -f *~ *.o mjs core *.core ktrace.out gmon.out DEADJOE include/*~
	make -C tools clean

install: all
	install -c -o 0 -g 0 mjs /usr/local/bin
	install -c -o 0 -g 0 tools/findmp3 /usr/local/bin
	touch /var/log/mp3log
	chmod a+rw /var/log/mp3log
	mkdir -p /var/state
	touch /var/state/mp3active
	chmod a+rw /var/state/mp3active
	touch /tmp/findresults
	chmod a+rw /tmp/findresults

	@echo -e "\nWarning: The config file format has been changed !!!"
	@echo -e "\nDon't forget to cp mjsrc.EXAMPLE to ~/.mjsrc and change it as needed !!!\nYou may also read INSTALL for further instructions.\n"
	@echo -e "(c) mvgalen 2003 mvgalen@users.sourceforge.net\n\n"
release: dist

dist: clean
	cd ..; tar cvzf mjs-$(VERSION).tar.gz --exclude CVS mjs

mostlyclean:
	rm -f *~ core *.core *.o
	make -C tools mostlyclean

# file dependencies

config.o: include/top.h include/defs.h include/colors.h include/struct.h
config.o: include/config.h include/extern.h
files.o: include/top.h include/defs.h include/colors.h include/struct.h
files.o: include/files.h include/misc.h include/info.h include/extern.h
files.o: include/list.h
info.o: include/top.h include/defs.h include/struct.h include/info.h
info.o: include/misc.h include/window.h include/extern.h
inputline.o: include/top.h include/defs.h include/struct.h
inputline.o: include/inputline.h
misc.o: include/top.h include/defs.h include/colors.h include/struct.h
misc.o: include/misc.h include/window.h include/extern.h
mjs.o: include/top.h include/defs.h include/colors.h include/struct.h
mjs.o: include/mjs.h include/playlist.h include/window.h include/misc.h
mjs.o: include/files.h include/mpgcontrol.h include/config.h
mjs.o: include/inputline.h include/extern.h
mpgcontrol.o: include/top.h include/mpgcontrol.h include/misc.h
mpgcontrol.o: include/window.h include/mjs.h include/playlist.h
mpgcontrol.o: include/extern.h
playlist.o: include/top.h include/mpgcontrol.h include/misc.h
playlist.o: include/window.h include/mjs.h include/extern.h
playlist.o: include/list.h include/files.h include/struct.h
tokens.o: include/top.h include/defs.h include/struct.h include/tokens.h
tokens.o: include/extern.h
window.o: include/top.h include/defs.h include/colors.h include/struct.h
window.o: include/window.h include/misc.h include/tokens.h include/files.h
window.o: include/extern.h
list.o: include/top.h include/defs.h include/struct.h include/misc.h
