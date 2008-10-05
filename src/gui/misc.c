#include "defs.h"
#include "misc.h"
#include "gui.h"

#include <string.h>

/*
 * There are 64 possible color_pairs, 0-63. This is 6 bits worth of colors,
 * and since there are only 64 possible color combos then why not use them
 * all for easy translation. So, in 8 bits, the format is as follows:
 * 00xxxyyy, where xxx is the foreground color (bits) and yyy is the background
 * color. Everything starts at zero of course, otherwise that wouldn't fit
 * naturally into our 0-63 scheme.
 * However, ncurses hard-codes the color pair zero to be what we would call
 * 56 (white on black, 7<<3), so we fix color pair 56 to be black on black
 * (what 0<<3|0 would have been). Doh.
 */

void
init_ansi_pair ( void )
{
	u_int8_t fore, back;
	for ( fore = 0; fore < COLORS; fore++ )
		for ( back = 0; back < COLORS; back++ )
			init_pair ( fore<<3 | back, fore, back );
}

int
my_mvwprintw ( WINDOW *win, int y, int x, u_int32_t attribs, const u_char *format, ... )
{
	u_int16_t i;
	va_list args;
	u_char buf[BUFFER_SIZE+1], *p;
	memset ( buf, 0, BUFFER_SIZE+1 );
	va_start ( args, format );
	i = vsnprintf ( buf, BUFFER_SIZE, format, args );
	va_end ( args );
	wmove ( win, y, x );
	p = buf;
	while ( *p )
		waddch ( win, *p++ | attribs );
	return i;
}

// int
// my_wprintw(WINDOW *win, u_int32_t attribs, const u_char *format, ...)
// {
// 	u_int16_t i;
// 	va_list args;
// 	u_char buf[BUFFER_SIZE+1], *p;
// 	memset(buf, 0, BUFFER_SIZE+1);
// 	va_start(args, format);
// 	i = vsnprintf(buf, BUFFER_SIZE, format, args);
// 	va_end(args);
// 	p = buf;
// 	while (*p)
// 		waddch(win, *p++ | attribs);
// 	return i;
// }

int
my_mvwnprintw ( WINDOW *win, int y, int x, u_int32_t attribs, int n, const u_char *format, ... )
{
	u_int16_t i;
	va_list args;
	u_char buf[n+1], *p;
	memset ( buf, 0, n+1 );
	va_start ( args, format );
	i = vsnprintf ( buf, n, format, args );
	va_end ( args );
	wmove ( win, y, x );
	p = buf;
	while ( *p )
		waddch ( win, *p++ | attribs );
	return i;
}

int
my_mvwnprintw2 ( WINDOW *win, int y, int x, u_int32_t attribs, int n, const u_char *format, ... )
{
	u_int16_t i;
	va_list args;
	u_char buf[n+1], *p;
	memset ( buf, 0, n+1 );
	va_start ( args, format );
	i = vsnprintf ( buf, n, format, args );
	va_end ( args );
	wmove ( win, y, x );
	p = buf;
	while ( *p )
	{
		waddch ( win, *p++ | attribs );
		n--;
	}
	while ( n )
	{
		waddch ( win, ' ' | attribs );
		n--;
	}
	return i;
}

int
my_wnprintw ( WINDOW *win, u_int32_t attribs, int n, const u_char *format, ... )
{
	u_int16_t i;
	va_list args;
	u_char buf[n+1], *p;
	memset ( buf, 0, n+1 );
	va_start ( args, format );
	i = vsnprintf ( buf, n, format, args );
	va_end ( args );
	p = buf;
	while ( *p )
		waddch ( win, *p++ | attribs );
	return i;
}

int
my_mvwnaddstr ( WINDOW *win, int y, int x, u_int32_t attribs, size_t n, const u_char *str, int offset )
{
	u_char *s = ( u_char * ) str;
	size_t n2 = n;
	int offset_count = offset;
	for ( ; *s && offset_count; offset_count-- )
		*s++;

	wmove ( win, y, x );

	if ( str && *str )
	{
		for ( ; *s && n; n-- )
		{
			waddch ( win, *s++ | attribs );
		}
		if ( offset && n && strlen ( str ) + 1 > n2 )
		{
			int newOffset = offset + 1;
			if ( n == n2 )
			{
				//Overflow reset 0;
				newOffset = 0;
			}
			waddch ( win, ' ' | attribs );
			s = ( u_char * ) str;
			for ( ; *s && n - 1; n-- ) //Don't forget to subtract 1 of the space :P
			{
				waddch ( win, *s++ | attribs );
			}
			return newOffset;
		}
		else if ( strlen ( str ) + 1 > n2 )
		{
			return offset + 1;
		}
	}
	if ( n )
	{
		for ( ; n; n-- )
			waddch ( win, ' ' | attribs );
		return 0;
	}
	return OK;
}

// int
// my_wnaddstr(WINDOW *win, u_int32_t attribs, size_t n, const u_char *str)
// {
// 	u_char *s = (u_char *)str;
//
// 	if (str && *str)
// 		for (; *s && n; n--)
// 			waddch(win, *s++ | attribs);
// 	for (; n; n--)
// 		waddch(win, ' ' | attribs);
// 	return OK;
// }

int
my_waddstr ( WINDOW *win, u_int32_t attribs, const u_char *str )
{
	u_char *s = ( u_char * ) str;

	if ( !str || !*str )
		return OK;
	for ( ; *s; waddch ( win, *s++ | attribs ) );
	return OK;
}

int
my_mvwaddstr ( WINDOW *win, int y, int x, u_int32_t attribs, const u_char *str )
{
	wmove ( win, y, x );
	return my_waddstr ( win, attribs, str );
}

__inline__ void
my_wnclear ( WINDOW *win, int n )
{
	int x, y;
	getyx ( win, y, x );
	for ( ; n > 0; n-- )
		waddch ( win, ' ' );
	wmove ( win, y, x );
}

__inline__ void
my_mvwnclear ( WINDOW *win, int y, int x, int n )
{
	wmove ( win, y, x );
	for ( ; n > 0; n-- )
	{
		waddch ( win, ' ' );
	}
	wmove ( win, y, x );
}


