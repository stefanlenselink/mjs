/* external stuff */

extern Input *inputline;
extern Window *files, *info, *play, *active;
WINDOW *menubar;
extern pid_t pid;
extern struct sigaction handler1, handler2;
extern int p_status, inpipe[], errpipe[];
