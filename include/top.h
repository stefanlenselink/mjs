#ifdef __linux__
#define __USE_UNIX98
#define _GNU_SOURCE
#define __USE_GNU
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#ifndef FREEBSD_NCURSES_PORT
#include <ncurses.h>
#include <panel.h>
#else
#include <ncurses/ncurses.h>
#include <ncurses/panel.h>
#endif
#ifdef GPM_SUPPORT
#include <gpm.h>
#endif
