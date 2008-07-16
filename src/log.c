#include "log.h"

#include <syslog.h>

void log_init(void)
{
  openlog("MJS", LOG_CONS | LOG_NDELAY | LOG_PID, LOG_DAEMON);
}

void log_debug(char * msg)
{
  syslog(LOG_ERR, msg);
}

void log_debug_format(const char *format, va_list ap)
{
  syslog(LOG_ERR, format, ap);
}

void log_shutdown(void)
{
  closelog();
}
