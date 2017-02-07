#ifndef _dirstack_h
#define _dirstack_h

typedef struct _dirstack
{
	struct _dirstack *prev;
	char *fullpath;
	char *filename;
} dirstack;

int	dirstack_empty( void );
void dirstack_push( const char *, const char * );
void dirstack_pop( void );
char * dirstack_fullpath( void );
char * dirstack_filename( void );


#endif //_dirstack_h
