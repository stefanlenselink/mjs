#ifndef _mms_gpm_h
#define _mms_gpm_h

#define BETWEEN(n,start,size) (n > start+1 && n < start+size)

void	gpm_init(void);
void	gpm_close(void);

#endif /* _mms_gpm_h */
