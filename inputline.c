#include "defs.h"
#include "mms.h"
#include "struct.h"
#include "proto.h"

/* hide these babies from everyone else */

static int del_char (Input *, int);
static int add_char (Input *, int);
static int del_word (Input *);

/* Add a char at the current position */

static int
add_char (Input *input, int c)
{
	if ((input->len < BUFFER_SIZE) && isprint(c)) {
		if (input->pos <= input->len) { /* We are inserting a char */
			memmove (&input->buf[input->pos], &input->buf[input->pos-1], input->len-input->pos+1);
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
del_char (Input *input, int backspace)
{
	int len = input->len, pos = input->pos;
	if (len > 0) {
		if (backspace) {
			if (pos > 1) {
				if (pos <= len)
					memmove (&input->buf[pos-2], &input->buf[pos-1], len-pos+1);
				input->buf[--input->len] = '\0';
				input->pos--;
				return 1;
			}
		}
		else {
			if (pos <= len) {
				memmove (&input->buf[pos-1], &input->buf[pos], len-pos+1);
				input->buf[--input->len] = '\0';
				return 1;
			}
		}
	}
	return 0;
}

/* Delete the word (including trailing spaces) before position pos */

static int
del_word (Input *input)
{
	int i = 0, len = input->len, pos = input->pos;
	char *p, *s;
	if (len > 0) {
		if (pos > 1) {
			/* skip leading spaces, find the first space afterwards */
			s = (char *)input->buf;
			p = (char *)(input->buf + (pos - 1));
			if (!*p) {
				p--;
				i++;
			}
			while ((p > s) && (isspace(*p) || ispunct(*p))) {
				p--;
				i++;
			}
			while ((p > s) && !(isspace(*p) || ispunct(*p))) {
				p--;
				i++;
			}
			if (p == s) { /* oops, we hit the start of the buffer */
				memset(s, 0, len); /* we know the length, why bother zero'ing everything? */
				input->len = 0;
				input->pos = 1;
			}
			else {
				memmove(++p, p+i--, len-pos+2);
				memset(s+len-i, 0, i);
				input->len -= i;
				input->pos -= i;
			}
			return i;
		}
	}
	return 0;
}

int
do_inputline (Input *inputline)
{
	int i, c, alt = 0;
	c = wgetch(inputline->win);
	if (c == 27) {              /* damn alt key */
		alt = 1;
		c = wgetch(inputline->win);
	}
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
			if (alt) {
				if ((i = del_word(inputline)))
					inputline->update(inputline);
			}
			else {
				if (del_char(inputline, 1))
					inputline->update(inputline);
			}
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
		default:
			if (add_char(inputline, c))
				inputline->update(inputline);
			break;
	}
	return 1;
}

Input *
update_anchor (Input *inputline)
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
