CC = gcc
prefix = /usr/local/bin

SRCS =	mms.c misc.c info.c songs.c playlist.c inputline.c mpgcontrol.c \
	config.c id3.c
OBJS =	mms.o misc.o info.o songs.o playlist.o inputline.o mpgcontrol.o \
	config.o id3.o
INCLUDES = -I/usr/local/include
LIBRARY = -L/usr/local/lib
PROFILE = #-pg
LIBS = -lpanel -lncurses
ARCHFLAGS = -mpentium -march=pentium
WARNINGS = -Wall -Wbad-function-cast -Wcast-align
# Change this to -g for debugging
OPTFLAGS = -g3
CFLAGS = $(OPTFLAGS) $(PROFILE) $(WARNINGS) $(ARCHFLAGS)

PROGRAM = mms
VERSION = 0.86

# Uncomment these for GPM support
#SRCS += gpm.c
#OBJS += gpm.o
#CFLAGS += -DGPM_SUPPORT
#LIBS += -lgpm


all: $(PROGRAM)

default: all

$(OBJS): struct.h proto.h defs.h
mms.o misc.o:   colors.h
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

$(PROGRAM): $(OBJS)
	$(CC) $(PROFILE) $(ARCHFLAGS) -o $(PROGRAM) $(OBJS) $(LIBRARY) $(LIBS)

static : $(OBJS)
	$(CC) -static $(PROFILE) $(ARCHFLAGS) -o $(PROGRAM) $(OBJS) $(LIBRARY) $(LIBS)

clean:
	rm -f *~ *.o $(PROGRAM) core mms.core DEADJOE

install: all
	install -c -o 0 -g 0 mms /usr/local/bin

release: dist

dist: clean
	cd ..; tar cvzf mms-$(VERSION).tgz mms-$(VERSION)

mostlyclean:
	rm -f *~ core *.o
