#include "defs.h"
#include "mjs.h"
#include "gui.h"
#include "misc.h"
#include "config.h"
#include "inputline.h"

#include <string.h>

/* TODO volgens mij word question nooit meer vrijgegeven */

Window * question;
void
ask_question(char *title, char *prompt, char *initial, Window (*callback)(Window *))
{
	question = calloc(1, sizeof(Window));
	question->win = newwin(3, 60, 9, 15);
	question->width = 60;
	question->height = 3;
	question->y = 9;
	question->x = 15;
	question->activate = active_win;
	question->deactivate = inactive_win;
	question->title = strdup(title); /* dont forget to free this */
	question->flags |= W_RDONLY;
}
