CC = gcc

SRCS = mms.c misc.c info.c songs.c playlist.c inputline.c mpgcontrol.c
OBJS = mms.o misc.o info.o songs.o playlist.o inputline.o mpgcontrol.o
INCLUDES = -I/usr/local/include
LIBRARY = -L/usr/local/lib
PROFILE = #-pg
LIBS = -lncurses_g
ARCHFLAGS = -mpentium -march=pentium
WARNINGS = -Wall -Wbad-function-cast -Wcast-align
CFLAGS = -g3 $(PROFILE) $(WARNINGS) $(ARCHFLAGS)

PROGRAM = mms

all: $(PROGRAM)

default: all

$(OBJS): struct.h proto.h defs.h
mms.o misc.o:   colors.h
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

$(PROGRAM): $(OBJS)
	$(CC) $(PROFILE) $(ARCHFLAGS) -o $(PROGRAM) $(OBJS) $(LIBRARY) $(LIBS)

clean:
	rm -f *~ *.o $(PROGRAM) core mms.core DEADJOE

dist: clean
	cd ..; tar cvzf mms.tgz mms

mostlyclean:
	rm -f *~ core *.o
