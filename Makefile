CC = gcc
prefix = /usr/local/bin

PROGRAM = mms
VERSION = 0.89

SRCS =	mms.c misc.c info.c config.c playlist.c inputline.c mpgcontrol.c \
	id3.c tokens.c window.c files.c
OBJS =	mms.o misc.o info.o config.o playlist.o inputline.o mpgcontrol.o \
	id3.o tokens.o window.o files.o
INCLUDES = -I/usr/local/include -I$(PWD)/include
LIBRARY = -L/usr/local/lib
PROFILE = #-pg
LIBS = -lncurses -lpanel
#ARCHFLAGS = -mpentium
WARNINGS = -Wall -Wbad-function-cast -Wcast-align
#OPTFLAGS = -O2
# Comment this out for debugging
OPTFLAGS += -g3

CFLAGS = $(OPTFLAGS) $(PROFILE) $(WARNINGS) $(ARCHFLAGS) -DVERSION=\"$(VERSION)\"

# Uncomment these for GPM support
#SRCS += gpm.c
#OBJS += gpm.o
#CFLAGS += -DGPM_SUPPORT
#LIBS += -lgpm

# Uncomment these if you use FreeBSD and their weird ncurses port
#CFLAGS += -DFREEBSD_NCURSES_PORT

all: $(PROGRAM)

default: all

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

$(PROGRAM): $(OBJS)
	$(CC) $(PROFILE) $(ARCHFLAGS) -o $(PROGRAM) $(OBJS) $(LIBRARY) $(LIBS)

static : $(OBJS)
	$(CC) -static $(PROFILE) $(ARCHFLAGS) -o $(PROGRAM) $(OBJS) $(LIBRARY) $(LIBS)

clean:
	rm -f *~ *.o $(PROGRAM) core *.core ktrace.out gmon.out DEADJOE 

install: all
	install -c -o 0 -g 0 mms /usr/local/bin

release: dist

dist: clean
	cd ..; tar cvzf mms-$(VERSION).tgz mms-$(VERSION)

mostlyclean:
	rm -f *~ core *.core *.o

# file dependencies

config.o: include/top.h include/defs.h include/colors.h include/struct.h
config.o: include/config.h include/extern.h
files.o: include/top.h include/defs.h include/colors.h include/struct.h
files.o: include/files.h include/misc.h include/info.h include/extern.h
gpm.o: include/top.h include/defs.h include/struct.h include/extern.h
gpm.o: include/mms_gpm.h include/misc.h
id3.o: include/top.h include/defs.h include/struct.h include/extern.h
id3.o: include/misc.h include/inputline.h include/id3.h
info.o: include/top.h include/defs.h include/struct.h include/info.h
info.o: include/misc.h include/window.h include/extern.h
inputline.o: include/top.h include/defs.h include/struct.h
inputline.o: include/inputline.h
misc.o: include/top.h include/defs.h include/colors.h include/struct.h
misc.o: include/misc.h include/window.h include/extern.h
mms.o: include/top.h include/defs.h include/colors.h include/struct.h
mms.o: include/mms.h include/playlist.h include/window.h include/misc.h
mms.o: include/files.h include/mpgcontrol.h include/id3.h include/config.h
mms.o: include/inputline.h include/extern.h
mpgcontrol.o: include/top.h include/mpgcontrol.h include/misc.h
mpgcontrol.o: include/window.h include/mms.h include/playlist.h
mpgcontrol.o: include/extern.h
playlist.o: include/top.h include/mpgcontrol.h include/misc.h
playlist.o: include/window.h include/mms.h include/extern.h
tokens.o: include/top.h include/defs.h include/struct.h include/tokens.h
tokens.o: include/extern.h
window.o: include/top.h include/defs.h include/colors.h include/struct.h
window.o: include/window.h include/misc.h include/tokens.h include/files.h
window.o: include/extern.h
