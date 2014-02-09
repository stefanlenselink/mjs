#include "log.h"

#include <syslog.h>
#include <stdio.h>
#include <stdarg.h>

FILE * logfile;
void log_init(void)
{
  //openlog("MJS", LOG_CONS | LOG_NDELAY | LOG_PID, LOG_DAEMON);
  logfile = fopen("/tmp/mjs4.log", "a");
}

void log_debug(char * msg)
{
  //syslog(LOG_ERR, "%s", msg);
  fprintf(logfile, "%s\n", msg);
}

void log_debug_format(const char *format, ...)
{
  //syslog(LOG_ERR, format, ap);
  va_list argp;
  va_start( argp, format );
  vfprintf(logfile, format, argp);
  va_end(argp);
}

void log_shutdown(void)
{
  //closelog();
  fclose(logfile);
}
