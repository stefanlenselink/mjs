#ifndef _log_h
#define _log_h

#include <stdarg.h>

void log_init(void);
void log_debug(char *);
void log_debug_format(const char *format, va_list ap);
void log_shutdown(void);

#endif /*_log_h*/
