#include "top.h"
#include "defs.h"
#include "struct.h"
#include "mpgcontrol.h"
#include "misc.h"
#include "window.h"
#include "mjs.h"
#include "playlist.h"
#include "extern.h"
#include "stdio.h"
#include "time.h"

static int inpipe[2], outpipe[2];
static struct sigaction handler;

static mpgreturn	*read_cmd (int, mpgreturn *);
static int		 mpg_output (int);

int
start_mpg_child(void)
{
	int i;

	if (pipe(inpipe) || pipe(outpipe))
		bailout(1);
	fcntl(outpipe[1], F_SETFD, O_NONBLOCK);
	handler.sa_handler = SIG_DFL;
	handler.sa_flags = 0;
	sigaction(SIGCHLD, &handler, NULL);

	errno = 0;
	switch (pid = fork()) {
		case -1:
			bailout(3);		
		case 0:
			dup2(inpipe[0], 0);
			dup2(outpipe[1], 1);
			inpipe[1] = outpipe[0] = -1;
			for (i = 2; i < 256; i++)
				close(i);
			errno = 0;
			if (conf->buffer > 0) {
				char buf[128];
				memset(buf, 0, sizeof(buf));
				snprintf(buf, 127, "%d", conf->buffer);
				if (conf->c_flags & C_MONO)
					execlp(conf->mpgpath, "mjs-output", "-m", "-b", buf, "-a", conf->output, "-R", "-", (char *)NULL);
				else 
					execlp(conf->mpgpath, "mjs-output", "-b", buf, "-a", conf->output, "-R", "-", (char *)NULL);
						
			} else
				if (conf->c_flags & C_MONO)
					execlp(conf->mpgpath, "mjs-output", "-m", "-a", conf->output, "-R", "-", (char *)NULL);
				else
					execlp(conf->mpgpath, "mjs-output", "-a", conf->output, "-R", "-", (char *)NULL);
		default:
			handler.sa_handler = (SIGHANDLER) restart_mpg_child;
			handler.sa_flags = SA_NOCLDSTOP | SA_RESTART;
			sigaction(SIGCHLD, &handler, NULL);
			
			break;
	}
	return pid;
}

void
add_player_descriptor(fd_set *fds)
{
	FD_SET(outpipe[0], fds);
}

void
check_player_output(fd_set *fds)
{
	if (FD_ISSET(outpipe[0], fds))
		mpg_output(outpipe[0]);
}

void
restart_mpg_child(void)
{
	wlist *win;

	/* we will use this to clean up on a SIGCHLD */
	if (pid)
		waitpid(pid, NULL, 0);
	pid = 0;
	
	start_mpg_child();
	
	win = play->contents.list;
	if (win && win->playing)
		play_next_song();
}

int send_cmd(int type, ...)
{
	char buf[512], *filename;
	FILE *log;
	FILE *active;
	time_t timevalue;
	int fd = inpipe[1];
	long int skip;
	va_list args;
	
	memset(buf, 0, 512);
	if (fd < 0)
		return 0;
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
			
			timevalue = time(NULL);
						
			active = fopen(conf->statefile,"w");
			if (active) {
				fprintf(active,"%s\n",filename);
				fclose(active);
			}
			log = fopen(conf->logfile,"a");
			if (log) {
				fprintf(log,"%.24s\t%s\n",ctime(&timevalue),filename);
				fclose(log);
			}
			
			break;
		case STOP:
			write(fd, "STOP\n", 5);
			timevalue = time(NULL);
			
			active = fopen(conf->statefile,"w");
			if (active) {
				fprintf(active,"%s","                      \n");
				fclose(active);
				}
			break;
		case PAUSE:
			write(fd, "PAUSE\n", 6);
			active = fopen(conf->statefile,"w");
			if (active) {
				fprintf(active,"%s","         * Pause *    \n");
				fclose(active);
				}
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

static mpgreturn *read_cmd(int fd, mpgreturn *status)
{
	char buf[512], c = '\0', *p = NULL, *s = NULL;
	extern int p_status;
	int n = 0, tmpstatus;

	memset(buf, 0, 512);
	memset(status, 0, sizeof(mpgreturn));
	/* this seems inefficient but 1) i dont want to use FILE * and 2) we only
	 * one to get one line at a time
	 */
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

static int
mpg_output(int fd)
{
	mpgreturn message;
	
	memset(&message, 0, sizeof(mpgreturn));
	if (read_cmd(fd, &message)) {
		show_playinfo(&message);
		return 1;
	}
	update_status();
	return 0;
}
