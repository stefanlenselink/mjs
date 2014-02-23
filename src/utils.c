#include "utils.h"

#include <string.h>
#include <ctype.h>


/**
 * Trims whitespace from string at s in place, moves start of string back to s.
 */
char * strtrim(char * s)
{
	char * front = s - 1; //Decrement for cleaner while loop.
	char * end = NULL;
	size_t len = 0;
	size_t newlen = 0;

	if( s == NULL )
		return NULL;

	if( *s == '\0' )
		return s;

	len = strlen(s);
	end = s + len;

	//Move front and end pointers forward/backward if whitespace found.
	while( isspace(*(++front)) )
	while( isspace(*(--end)) && end != front )


	if( s + len - 1 != end)
	{
		*(end + 1) = '\0';
	}
	else if( front != s && end == front )
	{
		*s = '\0';
		return s; //No need to copy back to front
	}


	//Copy resulting string back to start at s
	newlen = strlen(front);
	memmove( s, front, newlen+1);

	return s;
}
