CC = gcc
prefix = /usr/local/bin

VERSION = 3.3-rc2

OBJS = mjscount.o config_stripped.o ../info.o

INCLUDES = -I../include -Iinclude
LIBRARY = -L/usr/local/lib
PROFILE = #-pg
LIBS =
ARCHFLAGS = -mcpu=pentium
WARNINGS = -Wall -Wbad-function-cast -Wcast-align
OPTFLAGS = -O2
# Comment this out for debugging
OPTFLAGS += -g3

CFLAGS = $(OPTFLAGS) $(PROFILE) $(WARNINGS) $(ARCHFLAGS) -DVERSION=\"$(VERSION)\"

# Uncomment these if you use FreeBSD and their weird ncurses port
#CFLAGS += -DFREEBSD_NCURSES_PORT

all: mjscount

default: all

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

mjscount: $(OBJS)
	$(CC) $(PROFILE) $(ARCHFLAGS) -o mjscount $(OBJS) $(LIBRARY) $(LIBS)

static : $(OBJS)
	@echo "" 
	$(CC) -static $(PROFILE) $(ARCHFLAGS) -o mjscount $(OBJS) $(LIBRARY) $(LIBS)

clean:
	rm -f *~ *.o mjscount core *.core ktrace.out gmon.out DEADJOE

install: all
#	install -c -o 0 -g 0 findmp3 /usr/local/bin

mostlyclean:
	rm -f *~ core *.core *.o

# file dependencies

config_stripped.o: ../include/top.h ../include/defs.h ../include/struct.h
config_stripped.o: ../include/config.h 
mjscount.o: ../include/top.h ../include/struct.h ../include/config.h ../include/info.h