#ifndef spotifyctl_h
#define spotifyctl_h

#include <libspotify/api.h>

#define MAX_LINK 1024


int spotifyctl_run(void *erl_pid, const char *cache_location, const char *settings_location,
                   const char *username, const char *password);
void spotifyctl_set_pid(void *erl_pid);
int spotifyctl_has_current_track();

sp_track *spotifyctl_current_track();
sp_session *spotifyctl_get_session();

int spotifyctl_browse_album(const char *link, void *reference, char **error_msg);

#define CMD_STOP 1
#define CMD_PLAYER_LOAD     10
#define CMD_PLAYER_PLAY     11
#define CMD_PLAYER_PREFETCH 12
#define CMD_PLAYER_SEEK     13
#define CMD_PLAYER_UNLOAD   14
#define CMD_TRACK_INFO      15

#define CMD_RESULT_OK 0
#define CMD_RESULT_ERROR 1
#define CMD_RESULT_LOADING 2

int spotifyctl_do_cmd0(char cmd, char **error_msg);
int spotifyctl_do_cmd1(char cmd, void *arg1, char **error_msg);
int spotifyctl_do_cmd2(char cmd, void *arg1, void *arg2, char **error_msg);



// internal

void load_queue_add(void *reference, sp_track *track);
void load_queue_check();


#endif
