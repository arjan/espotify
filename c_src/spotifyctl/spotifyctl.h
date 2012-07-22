#ifndef spotifyctl_h
#define spotifyctl_h

#include <libspotify/api.h>

#define MAX_LINK 1024
#define MAX_QUERY 1024


int spotifyctl_run(void *erl_pid, const char *cache_location, const char *settings_location,
                   const char *username, const char *password);
void spotifyctl_set_pid(void *erl_pid);
int spotifyctl_has_current_track();
void spotifyctl_stop();

sp_track *spotifyctl_current_track();
sp_session *spotifyctl_get_session();

int spotifyctl_track_info(const char *link, void *reference, char **error_msg);
int spotifyctl_browse_album(const char *link, void *reference, char **error_msg);
int spotifyctl_browse_artist(const char *link, sp_artistbrowse_type type, void *reference, char **error_msg);
int spotifyctl_load_image(const char *link, void *reference, char **error_msg);

typedef struct {
    char query[MAX_QUERY];
    unsigned int track_offset;
    unsigned int track_count;
    unsigned int album_offset;
    unsigned int album_count;
    unsigned int artist_offset;
    unsigned int artist_count;
    unsigned int playlist_offset;
    unsigned int playlist_count;
    sp_search_type search_type;
} spotifyctl_search_query;

void spotifyctl_search(spotifyctl_search_query query, void *reference);

int spotifyctl_load_user_playlistcontainer(const char *user, void *reference, char **error_msg);
int spotifyctl_load_playlistcontainer(void *reference, char **error_msg);
int spotifyctl_load_playlist(const char *link, void *reference, char **error_msg);

#define CMD_STOP 1
#define CMD_PLAYER_LOAD     10
#define CMD_PLAYER_PLAY     11
#define CMD_PLAYER_PREFETCH 12
#define CMD_PLAYER_SEEK     13
#define CMD_PLAYER_UNLOAD   14

#define CMD_RESULT_OK 0
#define CMD_RESULT_ERROR 1
#define CMD_RESULT_LOADING 2

int spotifyctl_do_cmd0(char cmd, char **error_msg);
int spotifyctl_do_cmd1(char cmd, void *arg1, char **error_msg);


// internal

typedef enum load_queue_type {
    Q_LOAD_TRACK = 1,
    Q_LOAD_PLAYLIST_TRACK = 2
} load_queue_type;

void load_queue_add(load_queue_type type, void *reference, sp_track *track, sp_playlist *playlist);
void load_queue_check();


#endif
