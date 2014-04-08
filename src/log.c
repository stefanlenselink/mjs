#include "log.h"
#include "config/config.h"

#include <syslog.h>
#include <stdio.h>
#include <stdarg.h>

extern Config *conf;

static FILE *logfile;

void log_init(void)
{
	if (conf->debugfile) {
		logfile = fopen(conf->debugfile, "a");
	}
}

void log_debug(char *msg)
{
	if (logfile)
		fprintf(logfile, "%s", msg);
}

void log_debug_format(const char *format, ...)
{
	va_list argp;

	if (!logfile)
		return;

	va_start(argp, format);
	vfprintf(logfile, format, argp);
	va_end(argp);
}

void log_shutdown(void)
{
	if (logfile)
		fclose(logfile);
}
