
#include <errno.h>
#include <libgen.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include <libspotify/api.h>

#include "audio.h"
#include "spotifyctl.h"
#include "spotifyctl_util.h"
#include "../espotify_async.h"

#define USER_AGENT "espotify"

#define DBG(d) (fprintf(stderr, "DEBUG: " d "\n"))

// see appkey.c
extern const char g_appkey[];
extern const size_t g_appkey_size;


typedef struct {
    
    /* The output queue for audo data */
    audio_fifo_t audiofifo;
    /* flag for audio initialization */
    int audio_initialized;
    
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

    // the pid object for communicating back
    void *erl_pid;

    // The global session handle
    sp_session *session;
    // The global user handle
    sp_user *user;
    // The playlist container
    sp_playlistcontainer *playlist_container;

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

    
} spotifyctl_state;

static spotifyctl_state g_state = {0};
    


/**
 * Called on various events to start playback if it hasn't been started already.
 *
 * The function simply starts playing the first track of the playlist.
 */

/* --------------------------  PLAYLIST CALLBACKS  ------------------------- */
/**
 * Callback from libspotifyctl, saying that a track has been added to a playlist.
 *
 * @param  pl          The playlist handle
 * @param  tracks      An array of track handles
 * @param  num_tracks  The number of tracks in the \c tracks array
 * @param  position    Where the tracks were inserted
 * @param  userdata    The opaque pointer
 */
static void tracks_added(sp_playlist *pl, sp_track * const *tracks,
                         int num_tracks, int position, void *userdata)
{
}

/**
 * Callback from libspotifyctl, saying that a track has been added to a playlist.
 *
 * @param  pl          The playlist handle
 * @param  tracks      An array of track indices
 * @param  num_tracks  The number of tracks in the \c tracks array
 * @param  userdata    The opaque pointer
 */
static void tracks_removed(sp_playlist *pl, const int *tracks,
                           int num_tracks, void *userdata)
{
}

/**
 * Callback from libspotifyctl, telling when tracks have been moved around in a playlist.
 *
 * @param  pl            The playlist handle
 * @param  tracks        An array of track indices
 * @param  num_tracks    The number of tracks in the \c tracks array
 * @param  new_position  To where the tracks were moved
 * @param  userdata      The opaque pointer
 */
static void tracks_moved(sp_playlist *pl, const int *tracks,
                         int num_tracks, int new_position, void *userdata)
{
}

/**
 * Callback from libspotifyctl. Something renamed the playlist.
 *
 * @param  pl            The playlist handle
 * @param  userdata      The opaque pointer
 */
static void playlist_renamed(sp_playlist *pl, void *userdata)
{
}

/**
 * The callbacks we are interested in for individual playlists.
 */
static sp_playlist_callbacks pl_callbacks = {
    .tracks_added = &tracks_added,
    .tracks_removed = &tracks_removed,
    .tracks_moved = &tracks_moved,
    .playlist_renamed = &playlist_renamed,
};


/* --------------------  PLAYLIST CONTAINER CALLBACKS  --------------------- */
/**
 * Callback from libspotifyctl, telling us a playlist was added to the playlist container.
 *
 * We add our playlist callbacks to the newly added playlist.
 *
 * @param  pc            The playlist container handle
 * @param  pl            The playlist handle
 * @param  position      Index of the added playlist
 * @param  userdata      The opaque pointer
 */
static void playlist_added(sp_playlistcontainer *pc, sp_playlist *pl,
                           int position, void *userdata)
{
}

/**
 * Callback from libspotifyctl, telling us a playlist was removed from the playlist container.
 *
 * This is the place to remove our playlist callbacks.
 *
 * @param  pc            The playlist container handle
 * @param  pl            The playlist handle
 * @param  position      Index of the removed playlist
 * @param  userdata      The opaque pointer
 */
static void playlist_removed(sp_playlistcontainer *pc, sp_playlist *pl,
                             int position, void *userdata)
{
    sp_playlist_remove_callbacks(pl, &pl_callbacks, NULL);
}


/**
 * Callback from libspotifyctl, telling us the rootlist is fully synchronized
 * We just print an informational message
 *
 * @param  pc            The playlist container handle
 * @param  userdata      The opaque pointer
 */
static void container_loaded(sp_playlistcontainer *pc, void *userdata)
{
    DBG("container loaded");
    fprintf(stderr, "spotifyctl: Rootlist synchronized (%d playlists)\n",
            sp_playlistcontainer_num_playlists(pc));

    if (g_state.playlist_container) {
        sp_playlistcontainer_release(g_state.playlist_container);
    }

    sp_playlistcontainer_add_ref(pc);
    g_state.playlist_container = pc;
}


/**
 * The playlist container callbacks
 */
static sp_playlistcontainer_callbacks pc_callbacks = {
    .playlist_added = 0,//&playlist_added,
    .playlist_removed = 0,//&playlist_removed,
    .playlist_moved = 0,//&playlist_removed,
    .container_loaded = &container_loaded,
};


/* ---------------------------  SESSION CALLBACKS  ------------------------- */
/**
 * This callback is called when an attempt to login has succeeded or failed.
 *
 * @sa sp_session_callbacks#logged_in
 */
static void logged_in(sp_session *sess, sp_error error)
{
    if (SP_ERROR_OK != error) {
        esp_error_feedback(g_state.erl_pid, "logged_in", sp_error_message(error));
        return;
    }

    sp_playlistcontainer *pc = sp_session_playlistcontainer(g_state.session);
    
    sp_playlistcontainer_add_callbacks(
        pc,
        &pc_callbacks,
        NULL);

    
    fprintf(stderr, "spotifyctl: Looking at %d playlists\n", sp_playlistcontainer_num_playlists(pc));

    int i;
    for (i = 0; i < sp_playlistcontainer_num_playlists(pc); ++i) {
        sp_playlist *pl = sp_playlistcontainer_playlist(pc, i);
        sp_playlist_add_callbacks(pl, &pl_callbacks, NULL);
    }

    g_state.user = sp_session_user(g_state.session);
    sp_user_add_ref(g_state.user);
    
    // Make login feedback
    sp_link *link = sp_link_create_from_user(g_state.user);
    char link_str[MAX_LINK];
    sp_link_as_string(link, link_str, MAX_LINK);
    
    esp_logged_in_feedback(g_state.erl_pid,
                           link_str,
                           sp_user_canonical_name(g_state.user),
                           sp_user_display_name(g_state.user));

    sp_link_release(link);
}

/**
 * This callback is called from an internal libspotifyctl thread to ask us to
 * reiterate the main loop.
 *
 * We notify the main thread using a condition variable and a protected variable.
 *
 * @sa sp_session_callbacks#notify_main_thread
 */
static void notify_main_thread(sp_session *sess)
{
    pthread_mutex_lock(&g_state.notify_mutex);
    g_state.notify_events = 1;
    pthread_cond_signal(&g_state.notify_cond);
    pthread_mutex_unlock(&g_state.notify_mutex);
}

/**
 * This callback is used from libspotifyctl whenever there is PCM data available.
 *
 * @sa sp_session_callbacks#music_delivery
 */
static int music_delivery(sp_session *sess, const sp_audioformat *format,
                          const void *frames, int num_frames)
{
    audio_fifo_t *af = &g_state.audiofifo;
    audio_fifo_data_t *afd;
    size_t s;

    if (num_frames == 0)
        return 0; // Audio discontinuity, do nothing

    pthread_mutex_lock(&af->mutex);

    /* Buffer one second of audio */
    if (af->qlen > format->sample_rate) {
        pthread_mutex_unlock(&af->mutex);

        return 0;
    }

    s = num_frames * sizeof(int16_t) * format->channels;

    afd = malloc(sizeof(*afd) + s);
    memcpy(afd->samples, frames, s);

    afd->nsamples = num_frames;

    afd->rate = format->sample_rate;
    afd->channels = format->channels;

    TAILQ_INSERT_TAIL(&af->q, afd, link);
    af->qlen += num_frames;

    pthread_cond_signal(&af->cond);
    pthread_mutex_unlock(&af->mutex);

    return num_frames;
}


/**
 * This callback is used from libspotifyctl when the current track has ended
 *
 * @sa sp_session_callbacks#end_of_track
 */
static void end_of_track(sp_session *sess)
{
    pthread_mutex_lock(&g_state.notify_mutex);
    g_state.playback_done = 1;
    g_state.notify_events = 1;
    pthread_cond_signal(&g_state.notify_cond);
    pthread_mutex_unlock(&g_state.notify_mutex);
}


/**
 * Callback called when libspotifyctl has new metadata available
 *
 * Not used in this example (but available to be able to reuse the session.c file
 * for other examples.)
 *
 * @sa sp_session_callbacks#metadata_updated
 */
static void metadata_updated(sp_session *sess)
{
    if (g_state.player_load_track && 
        sp_track_is_loaded(g_state.player_load_track)) {
        sp_session_player_load(g_state.session, g_state.player_load_track);
        if (g_state.current_track) {
            sp_track_release(g_state.current_track);
        }
        g_state.current_track = g_state.player_load_track;
        g_state.player_load_track = 0;
        spotifyctl_track *t = make_track(g_state.session, g_state.current_track);
        esp_player_load_feedback(g_state.erl_pid, t);
        release_track(t);
        return;
    }
}

/**
 * Notification that some other connection has started playing on this account.
 * Playback has been stopped.
 *
 * @sa sp_session_callbacks#play_token_lost
 */
static void play_token_lost(sp_session *sess)
{
    audio_fifo_flush(&g_state.audiofifo);

    if (g_state.current_track != NULL) {
        sp_session_player_unload(g_state.session);
        g_state.current_track = NULL;
    }
}

/**
 * The session callbacks
 */
static sp_session_callbacks session_callbacks = {
    .logged_in = &logged_in,
    .notify_main_thread = &notify_main_thread,
    .music_delivery = &music_delivery,
    .metadata_updated = &metadata_updated,
    .play_token_lost = &play_token_lost,
    .log_message = NULL,
    .end_of_track = &end_of_track,
};

static sp_session_config spconfig = {
	.api_version = SPOTIFY_API_VERSION,
	.application_key = g_appkey,
	.application_key_size = 0, // Set in main()
	.user_agent = "spotify-jukebox-example",
	.callbacks = &session_callbacks,
	NULL,
};

/* -------------------------  END SESSION CALLBACKS  ----------------------- */

int wait_for_cmd_result()
{
    int cmd_result;
    pthread_mutex_lock(&g_state.result_mutex);
    pthread_cond_wait(&g_state.result_cond, &g_state.result_mutex);
    cmd_result = g_state.cmd_result;
    pthread_mutex_unlock(&g_state.result_mutex);
    return cmd_result;
}

int spotifyctl_do_cmd0(char cmd, char **error_msg)
{
    pthread_mutex_lock(&g_state.notify_mutex);
    g_state.cmd = cmd;
    g_state.cmd_error_msg = error_msg;
    pthread_cond_signal(&g_state.notify_cond);
    pthread_mutex_unlock(&g_state.notify_mutex);
    return wait_for_cmd_result();
}

int spotifyctl_do_cmd1(char cmd, void *arg1, char **error_msg)
{
    pthread_mutex_lock(&g_state.notify_mutex);
    g_state.cmd = cmd;
    g_state.cmd_error_msg = error_msg;
    g_state.cmd_arg1 = arg1;
    pthread_cond_signal(&g_state.notify_cond);
    pthread_mutex_unlock(&g_state.notify_mutex);
    return wait_for_cmd_result();
}

//int spotifyctl_do_cmd1(spotifyctl_cmd cmd, void *arg1, char **error_msg);
//int spotifyctl_do_cmd2(spotifyctl_cmd cmd, void *arg1, void *arg2, char **error_msg);

void handle_cmd_player_load()
{
    sp_link *link;
    sp_error err;
    sp_track *track;

    if (g_state.player_load_track) {
        g_state.cmd_result = CMD_RESULT_ERROR;
        *g_state.cmd_error_msg = "Still loading a track";
        return;
    }

    link = sp_link_create_from_string(g_state.cmd_arg1);
    if (!link) {
        g_state.cmd_result = CMD_RESULT_ERROR;
        *g_state.cmd_error_msg = "Parsing track failed";
        return;
    }

    track = sp_link_as_track(link);
    if (!track) {
        sp_link_release(link);
        g_state.cmd_result = CMD_RESULT_ERROR;
        *g_state.cmd_error_msg = "Link is not a track";
        return;
    }
    sp_track_add_ref(track);
    sp_link_release(link);

    if (!sp_track_is_loaded(track)) {
        g_state.player_load_track = track;
        g_state.cmd_result = CMD_RESULT_LOADING;
        return;
    }

    err = sp_session_player_load(g_state.session, track);
    if (err != SP_ERROR_OK) {
        g_state.cmd_result = CMD_RESULT_ERROR;
        *g_state.cmd_error_msg = sp_error_message(err);
        return;
    }

    g_state.player_load_track = NULL;
    g_state.current_track = track;

    spotifyctl_track *t = make_track(g_state.session, g_state.current_track);
    esp_player_load_feedback(g_state.erl_pid, t);
    release_track(t);

    g_state.cmd_result = CMD_RESULT_OK;
}

// record the pid
void spotifyctl_set_pid(void *erl_pid)
{
    g_state.erl_pid = erl_pid;
}

int spotifyctl_has_current_track()
{
    return g_state.current_track != NULL;
}

int spotifyctl_run(void *erl_pid,
                   const char *cache_location,
                   const char *settings_location,
                   const char *username,
                   const char *password)
{
    sp_error err;
    int next_timeout = 0;

    spotifyctl_set_pid(erl_pid);
    
    // reinit
    if (!g_state.audio_initialized) {
        audio_init(&g_state.audiofifo);
        g_state.audio_initialized = 1;
    }

    if (!g_state.session)
    {
        /* Create session */
        spconfig.application_key_size = g_appkey_size;

        spconfig.cache_location = cache_location;
        spconfig.settings_location = settings_location;
        
        err = sp_session_create(&spconfig, &g_state.session);

        if (SP_ERROR_OK != err) {
            fprintf(stderr, "Unable to create session: %s\n",
                    sp_error_message(err));
            exit(1);
        }
    }

    pthread_mutex_init(&g_state.notify_mutex, NULL);
    pthread_cond_init(&g_state.notify_cond, NULL);

    pthread_mutex_init(&g_state.result_mutex, NULL);
    pthread_cond_init(&g_state.result_cond, NULL);
    
    sp_session_login(g_state.session, username, password, 0, NULL);

    g_state.running = 1; // let's go

    //DBG("Enter main loop");
    
    pthread_mutex_lock(&g_state.notify_mutex);
    while (g_state.running) {
        if (next_timeout == 0) {
            while(!g_state.cmd && !g_state.notify_events)
                pthread_cond_wait(&g_state.notify_cond, &g_state.notify_mutex);
        } else {
            struct timespec ts;

#if _POSIX_TIMERS > 0
            clock_gettime(CLOCK_REALTIME, &ts);
#else
            struct timeval tv;
            gettimeofday(&tv, NULL);
            TIMEVAL_TO_TIMESPEC(&tv, &ts);
#endif

            ts.tv_sec += next_timeout / 1000;
            ts.tv_nsec += (next_timeout % 1000) * 1000000;

            while(!g_state.cmd && !g_state.notify_events)
            {
                if(pthread_cond_timedwait(&g_state.notify_cond, &g_state.notify_mutex, &ts))
                    break;
            }
        }

        if (g_state.notify_events) {
            // Process libspotify events
            g_state.notify_events = 0;
            pthread_mutex_unlock(&g_state.notify_mutex);

            do {
                sp_session_process_events(g_state.session, &next_timeout);
            } while (g_state.running && next_timeout == 0);
            pthread_mutex_lock(&g_state.notify_mutex);
        }
        else if (g_state.cmd) {
            // Process nif commands
            char cmd = g_state.cmd;
            g_state.cmd = 0;
            pthread_mutex_unlock(&g_state.notify_mutex);

            g_state.cmd_result = CMD_RESULT_OK;
            *g_state.cmd_error_msg = 0;

            switch (cmd)
            {
            case CMD_STOP:
                g_state.running = 0;
                break;
            case CMD_PLAYER_LOAD:
                handle_cmd_player_load();
                break;
            case CMD_PLAYER_PLAY:
                sp_session_player_play(g_state.session, *((int *)g_state.cmd_arg1));
                g_state.cmd_result = CMD_RESULT_OK;
                break;
            case CMD_PLAYER_SEEK:
                sp_session_player_seek(g_state.session, *((int *)g_state.cmd_arg1));
                g_state.cmd_result = CMD_RESULT_OK;
                break;
            case CMD_PLAYER_UNLOAD:
                sp_session_player_unload(g_state.session);
                if (g_state.current_track != NULL) {
                    sp_track_release(g_state.current_track);
                    g_state.current_track = 0;
                }
                g_state.cmd_result = CMD_RESULT_OK;
                break;
            default:
                fprintf(stderr, "Unknown client command: %d\n\r", cmd);
                g_state.cmd_result = CMD_RESULT_ERROR;
                break;
            }
            pthread_cond_signal(&g_state.result_cond);

            pthread_mutex_lock(&g_state.notify_mutex);
        }
    }

    // Cleaning up
    if (g_state.playlist_container) {
        sp_playlistcontainer_release(g_state.playlist_container);
    }
    if (g_state.user) {
        sp_user_release(g_state.user);
    }

    //DBG("Exit main loop");

    return 0;
}
            
