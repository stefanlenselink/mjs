int get_input (int, u_char *, int);

void bailout (int);
void unsuspend (int);

void active_win (Window *);
void inactive_win (Window *);
void init_ansi_pair (void);
void play_next_song (void);
void process_return (wlist *);
void free_list (flist *);
void update_status (void);
void show_playinfo (mpgreturn *);
void restart_mpg_child (void);
void do_scrollbar (Window *);
__inline__ void clear_play_info (WINDOW *);
void change_active (Window *);

int my_waddstr (WINDOW *, u_int32_t, const u_char *);
int my_mvwaddstr (WINDOW *, int, int, u_int32_t, const u_char *);
int my_wnaddstr (WINDOW *, u_int32_t, size_t, const u_char *);
int my_mvwnaddstr (WINDOW *, int, int, u_int32_t, size_t, const u_char *);
int my_wprintw (WINDOW *, u_int32_t, const u_char *, ...);
int my_mvwprintw (WINDOW *, int, int, u_int32_t, const u_char *, ...);
int my_wnprintw (WINDOW *, u_int32_t, int, const u_char *, ...);
int my_mvwnprintw (WINDOW *, int, int, u_int32_t, int, const u_char *, ...);
int my_mvwnprintw2 (WINDOW *, int, int, u_int32_t, int, const u_char *, ...);

int show_list (Window *);
int update_info (Window *);
int jump_to_song (flist *);
int sort_mp3 (const void *, const void *);

int mpg_output (int);
int send_cmd (int fd, int type, ...);
mpgreturn *read_cmd (int fd, mpgreturn *);
int start_mpg_child(void);

int do_inputline (Input *);
int do_read_playlist (Input *);
int do_save_playlist(Input *);
int update_menu (Input *);
int write_playlist (wlist *, const char *);

wlist *add_to_playlist (wlist *, flist *);
wlist *sort_songs (wlist *);
wlist *stop_player (wlist *);
wlist *pause_player (wlist *);
wlist *jump_forward (wlist *);
wlist *jump_backward (wlist *);
wlist *randomize_list (wlist *);
wlist *read_playlist (wlist *, const char *);

flist *read_mp3_list (wlist *);
flist *mp3_info (char *, u_int32_t);
flist *read_songs (flist *);
flist *delete_selected (wlist *, flist *);

Input *update_anchor (Input *);

Window *move_selector (Window *, int);

void read_config(void);
void edit_tag(flist *);

#ifdef GPM_SUPPORT
void gpm_init(void);
void gpm_close(void);
#endif
