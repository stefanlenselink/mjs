#include "mms.h"
#include "defs.h"
#include "struct.h"
#include "proto.h"

int send_cmd (int fd, int type, ...)
{
	char buf[512], *filename;
	long int skip;
	va_list args;
	
	memset(buf, 0, 512);
	switch (type) {
		case QUIT:
			write(fd, "QUIT\n", 5);
			break;
		case LOAD:
			va_start(args, type);
			filename = va_arg(args, char *);
			snprintf(buf, 511, "LOAD %s\n", filename);
			write(fd, buf, strlen(buf));
			va_end(args);
			break;
		case STOP:
			write(fd, "STOP\n", 5);
			break;
		case PAUSE:
			write(fd, "PAUSE\n", 6);
			break;
		case JUMP:
			va_start(args, type);
			skip = va_arg(args, long int);
			snprintf(buf, 511, "JUMP %+ld\n", skip);
			write(fd, buf, strlen(buf));
			va_end(args);
			break;
		default:
			return 0;
	}
	return 1;
}


mpgreturn *read_cmd (int fd, mpgreturn *status)
{
	char buf[512], c = '\0', *p = NULL, *s = NULL;
	extern int p_status;
	int n = 0, tmpstatus;
	
	memset(buf, 0, 512);
	memset(status, 0, sizeof(mpgreturn));
	while ((n < 511) && read(fd, &c, 1)) {
		if (c != '\n')
			buf[n++] = c;
		else
			break;
	}
	if (*buf != '@')
		return NULL;
	switch (*(p = buf+1)) {
		case 'F':
			status->played = strtoul(++p, &s, 10);
			status->left = strtoul(s, &p, 10);
			status->elapsed = strtod(p, &s);
			status->remaining = strtod(s, &p);
			return status;
		case 'P':
			tmpstatus = strtoul(++p, &s, 10);
			if (tmpstatus == 0)
				p_status = 0;
			break;
		default:
			break;
	}
	return NULL;
}
