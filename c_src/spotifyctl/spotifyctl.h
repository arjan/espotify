#ifndef spotifyctl_h
#define spotifyctl_h

#include <libspotify/api.h>
#include "audio.h"

#define MAX_LINK 1024
#define MAX_QUERY 1024

#define CHECK_VALID_LINK(link) if (!link) { *error_msg = "Parsing link failed"; return CMD_RESULT_ERROR;}

int spotifyctl_run(void *async_state, const char *cache_location, const char *settings_location,
                   const char *username, const char *password);
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


struct load_queue {
    load_queue_type type;
    sp_track *track;
    sp_playlist *playlist;
    void *reference;
    struct load_queue *next;
};
typedef struct load_queue spotifyctl_load_queue;


typedef struct {
    
    /* The output queue for audo data */
    audio_fifo_t audiofifo;
    
    // Synchronization mutex for the main thread
    pthread_mutex_t notify_mutex;
    // Synchronization condition variable for the main thread
    pthread_cond_t notify_cond;

    // Synchronization variable telling the main thread to process events
    int notify_events;
    // Non-zero when a track has ended and the spotifyctl has not yet started a new one
    int playback_done;

    // Non-zero when running the main loop
    int running;

    // the async queue for communicating back
    void *async_state;

    // The global session handle
    sp_session *session;

    // The playlist container
    sp_playlistcontainer *playlistcontainer;

    // The currently playing track
    sp_track *current_track;
    // Currently loading track for player_load()
    sp_track *player_load_track;
    // Whether or not we are paused.
    bool paused;

    // the current command
    char cmd;
    void *cmd_arg1;
    void *cmd_arg2;

    int cmd_result;
    char **cmd_error_msg;

    // Synchronization mutex for the result thread
    pthread_mutex_t result_mutex;
    // Synchronization condition variable for the result thread
    pthread_cond_t result_cond;

    spotifyctl_load_queue *load_queue_head;
    spotifyctl_load_queue *load_queue_tail;

    int first_session; // non-zero when this is the first time this session is used
} spotifyctl_state;


void load_queue_add(load_queue_type type, void *reference, sp_track *track, sp_playlist *playlist);
void load_queue_check();

// playlists.c
void load_container(sp_playlistcontainer *container, void *reference);
int spotifyctl_load_playlist(const char *link_str, void *reference, char **error_msg);

#endif
