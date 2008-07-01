#include "defs.h"
#include "inputline.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
/* hide these babies from everyone else */

static int	del_char (Input *, int);
static int	add_char (Input *, int);

/* Add a char at the current position */

static int
add_char(Input *input, int c)
{
	if ((input->len < BUFFER_SIZE) && isprint(c)) {
		if (input->pos <= input->len) { /* We are inserting a char */
			memmove(&input->buf[input->pos], &input->buf[input->pos-1], input->len-input->pos+1);
			input->buf[input->pos-1] = c;
		}
		else
			input->buf[input->len] = c;
		input->len++;
		input->pos++;
		return 1;
	}
	return 0;
}

/* Delete the char at position pos */

static int
del_char(Input *input, int backspace)
{
	int len = input->len, pos = input->pos;
	if (len > 0) {
		if (backspace) {
			if (pos > 1) {
				if (pos <= len)
					memmove(&input->buf[pos-2], &input->buf[pos-1], len-pos+1);
				input->buf[--input->len] = '\0';
				input->pos--;
				return 1;
			}
		}
		else {
			if (pos <= len) {
				memmove(&input->buf[pos-1], &input->buf[pos], len-pos+1);
				input->buf[--input->len] = '\0';
				return 1;
			}
		}
	}
	return 0;
}



int
do_inputline(Input *inputline, int c, int alt)
{
	switch (c) {
		case KEY_ENTER:
		case '\n':
		case '\r':
			if (!alt)
				inputline->finish(inputline);
			break;
		case KEY_DC:
			if (del_char(inputline, 0))
				inputline->update(inputline);
			break;
		case KEY_BACKSPACE:
			if (del_char(inputline, 1))
				inputline->update(inputline);
			break;
		case KEY_LEFT:
			if ((inputline->len > 0) && (inputline->pos > 1)) {
				inputline->pos--;
				inputline->update(inputline);
			}
			break;
		case KEY_RIGHT:
			if (inputline->pos <= inputline->len) {
				inputline->pos++;
				inputline->update(inputline);
			}
			break;
		case KEY_HOME:
			inputline->pos = 1;
			inputline->update(inputline);
			break;
		case KEY_END:
			inputline->pos = inputline->len + 1;
			inputline->update(inputline);
			break;
		case '\t':
			if (inputline->complete && inputline->complete(inputline))
				inputline->update(inputline);
		default:
			if (add_char(inputline, c))
				inputline->update(inputline);
			break;
	}
	return 1;
}

Input *
update_anchor(Input *inputline)
{
	int i, j, k;
	j = inputline->pos-5;
	i = j / (inputline->flen-10);
	k = j % (inputline->flen-10);
	if (i > 0) {
		inputline->anchor = inputline->buf + i*(inputline->flen-10);
		inputline->fpos = k+5;
	} else {
		inputline->anchor = inputline->buf;
		inputline->fpos = inputline->pos;
	}
	return inputline;
}

int
dummy_complete(Input *input)
{
	return 0;
}

/* return 1 if name is a directory, else 0 */
static int
is_dir(char *name)
{
	struct stat s;

	if (stat(name, &s) == -1)
		return 0;
	return S_ISDIR(s.st_mode);
}

/* tab completion for filenames */
int
filename_complete(Input *input)
{
	struct dirent **files;
	char dir[256], file[256], *p;
	int retval, i, num_files, num_matches, *matches;

	strncpy(dir, input->buf, sizeof(dir)-1);
	if ((p = strrchr(dir, '/')) == NULL) {
		dir[0] = '.', dir[1] = '\0'; /* current directory if none specified */
		strncpy(file, input->buf, sizeof(file)-1);
	}
	else {
		*p = '\0';
		strncpy(file, p+1, sizeof(file)-1);
	}
	if (dir[0] == '\0')  /* /foo */
		dir[0] = '/', dir[1] = '\0';

	if ((num_files = scandir(dir, &files, 0, alphasort)) < 0)
		return 0;

	retval = 0;
	if ((matches = calloc(num_files, sizeof(int))) == NULL)
		goto free_dir;

again:
	for (i = num_matches = 0; i < num_files; i++)
		if (!strncmp(file, files[i]->d_name, strlen(file)))
			matches[num_matches++] = i;
	if (num_matches == 0)
		goto free_dir;
	else if (num_matches == 1) {
		if (!strcmp(dir, "/"))
			sprintf(input->buf, "/%s", files[matches[0]]->d_name);
		else
			sprintf(input->buf, "%s/%s", dir, files[matches[0]]->d_name);
		input->len = strlen(input->buf);
		input->pos = input->fpos = input->len+1;
	}
	else {
		for (i = 0; i < num_matches-1; i++) {
			int a = matches[i], b = matches[i+1];
			if (strlen(files[a]->d_name) <= input->len
					|| strlen(files[b]->d_name) <= input->len
					|| files[a]->d_name[input->len] != files[b]->d_name[input->len])
			{
				retval = 1;
				goto free_dir;
			}
		}
		input->buf[input->len] = files[matches[0]]->d_name[input->len];
		input->len++;
		input->pos++;
		input->fpos++;
		goto again;
	}
	if (is_dir(input->buf) && input->len < BUFFER_SIZE) {
		input->buf[input->len++] = '/';
		input->buf[input->len] = '\0';
		input->pos++;
		input->fpos++;
	}
	retval = 1;

free_dir:
	for (i = 0; i < num_files; i++)
		free(files[i]);
	free(files);
	return retval;
}
