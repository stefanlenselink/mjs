/* external stuff */

extern Window *files, *info, *play, *active, *menubar, *id3box, *old_active;
extern Config *conf;
extern pid_t pid;
extern struct sigaction handler1, handler2;
extern int p_status, inpipe[2], errpipe[2];
extern char version_str[];
extern char *Genres[255];

/* colors */
extern u_int32_t colors[];
