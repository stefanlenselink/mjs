#include "dirstack.h"

#include <string.h>
#include <stdlib.h>

static dirstack *dirstack_top = NULL;

int dirstack_empty ( void )
{
	if ( dirstack_top )
		return 0;
	else
		return 1;
}

void dirstack_push ( const char *fullpath, const char *filename )
{
	dirstack *tmp = malloc( sizeof ( dirstack ) );
	memset(tmp, 0, sizeof(dirstack));
	
	tmp->fullpath = strdup ( fullpath );
	tmp->filename = strdup ( filename );
	tmp->prev = dirstack_top;
	
	dirstack_top = tmp;
}

void dirstack_pop ( void )
{
	dirstack *tmp;
	if ( dirstack_top )
	{
		tmp = dirstack_top;
		free ( tmp->fullpath );
		free ( tmp->filename );
		dirstack_top = tmp->prev;
		free ( tmp );
	}
	else
	{
		abort();
	}
}

char * dirstack_fullpath ( void )
{
	if ( dirstack_top )
		return dirstack_top->fullpath;
	else
		abort();

	return NULL; //This will never happen
}

char * dirstack_filename ( void )
{
	if ( dirstack_top )
		return dirstack_top->filename;
	else
		abort();
	
	return NULL; //This will never happen
}




