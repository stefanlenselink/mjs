/* external stuff */

extern Input *inputline;
extern Window *files, *info, *play, *active, *menubar, *editbox;
extern pid_t pid;
extern struct sigaction handler1, handler2;
extern int p_status, inpipe[2], errpipe[2];
extern char version_str[];
extern char mpgpath[256];
extern char dfl_plist[256];

extern int sel_advance;
extern int skip_info_box;
extern int loop;

/* colors */
extern u_int32_t colors[15];
