#ifndef _mms_h
#define _mms_h

#include "engine/engine.h"
#include "gui/gui.h"

void	bailout ( int );
void	unsuspend ( int );

void	update_status ( void );

void	ask_question ( char *, char *, char *, Window ( * ) ( Window * ) );
void    refresh_window ( int );

#endif /* _struct_h */
