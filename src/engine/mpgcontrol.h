#ifndef _mpgcontrol_h
#define _mpgcontrol_h

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
typedef struct {
  int played;
  int left;
  double elapsed;
  double remaining;
} mpgreturn;

void	restart_mpg_child(void);
int	start_mpg_child(void);
void	add_player_descriptor(fd_set *);
void	check_player_output(fd_set *);
int	send_cmd(int type, ...);

#endif /* _mpgcontrol_h */
