#ifndef _inputline_h
#define _inputline_h

#include "gui/gui.h"

int	 do_inputline (Input *, int, int);
Input	*update_anchor (Input *);
int	 dummy_complete(Input *);
int	 filename_complete(Input *);

#endif /* _inputline_h */
