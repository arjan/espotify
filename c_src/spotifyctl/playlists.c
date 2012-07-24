#include <libspotify/api.h>

#include "spotifyctl.h"
#include "../espotify_callbacks.h"

extern spotifyctl_state g_state;

typedef struct container_load_data {
    void *reference;
    sp_playlistcontainer *container;
} container_load_data;



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


static sp_playlist_callbacks load_playlist_callbacks;

static void pl_state_change(sp_playlist *pl, void *userdata)
{
    if (sp_playlist_is_loaded(pl)) {
        sp_playlist_remove_callbacks(pl, &load_playlist_callbacks, userdata);
        int i;
        int all_loaded = 1;
        for (i=0; i<sp_playlist_num_tracks(pl); i++) {
            sp_track *track = sp_playlist_track(pl, i);
            if (sp_track_is_loaded(track)) continue;
            all_loaded = 0;
            sp_track_add_ref(track);
            load_queue_add(Q_LOAD_PLAYLIST_TRACK, userdata, track, pl);
        }
        if (all_loaded) {
            esp_player_load_playlist_feedback(g_state.async_state, g_state.session, userdata, pl);
            sp_playlist_release(pl);
        }
    }
}


static sp_playlist_callbacks load_pc_playlist_callbacks;

static void pl_pc_state_change(sp_playlist *pl, void *userdata)
{
    if (sp_playlist_is_loaded(pl)) {
        container_load_data *data = (container_load_data *)userdata;
        sp_playlistcontainer *pc = data->container;

        sp_playlist_remove_callbacks(pl, &load_pc_playlist_callbacks, userdata);
        sp_playlist_release(pl);
        int i;
        int all_loaded = 1;
        for (i=0; i<sp_playlistcontainer_num_playlists(pc); i++) {
            if (sp_playlistcontainer_playlist_type(pc, i) == SP_PLAYLIST_TYPE_PLAYLIST) {
                if (!sp_playlist_is_loaded(sp_playlistcontainer_playlist(pc, i))) {
                    all_loaded = 0;
                }
            }        
        }
        if (all_loaded) {
            esp_player_load_playlistcontainer_feedback(g_state.async_state, g_state.session, data->reference, pc);
            sp_playlistcontainer_release(pc);
            free(data);
        }
    }
}

static sp_playlist_callbacks load_pc_playlist_callbacks = {
	NULL,
	NULL,
	NULL,
	NULL,
	pl_pc_state_change,
};


/* --------------------  PLAYLIST CONTAINER CALLBACKS  --------------------- */

static void container_loaded(sp_playlistcontainer *pc, void *userdata)
{
    int i;
    int all_loaded = 1;
    for (i=0; i<sp_playlistcontainer_num_playlists(pc); i++) {
        if (sp_playlistcontainer_playlist_type(pc, i) == SP_PLAYLIST_TYPE_PLAYLIST) {
            sp_playlist *pl = sp_playlistcontainer_playlist(pc, i);
            if (!sp_playlist_is_loaded(pl)) {
                all_loaded = 0;
                sp_playlist_add_ref(pl);
                sp_playlist_add_callbacks(pl, &load_pc_playlist_callbacks, userdata);
            }
        }        
    }
    if (all_loaded) {
        container_load_data *data = (container_load_data *)userdata;
        esp_player_load_playlistcontainer_feedback(g_state.async_state, g_state.session, data->reference, pc);
        sp_playlistcontainer_release(pc);
        free(data);
    }
}

static sp_playlistcontainer_callbacks pc_callbacks = {
    .playlist_added = 0,//&playlist_added,
    .playlist_removed = 0,//&playlist_removed,
    .playlist_moved = 0,//&playlist_removed,
    .container_loaded = &container_loaded,
};


static sp_playlist_callbacks load_playlist_callbacks = {
	NULL,
	NULL,
	NULL,
	NULL,
	pl_state_change,
};


void load_container(sp_playlistcontainer *container, void *reference) {
    container_load_data *load_data = (container_load_data *)malloc(sizeof(container_load_data));
    load_data->container = container;
    load_data->reference = reference;

    sp_playlistcontainer_add_ref(container);
    sp_playlistcontainer_add_callbacks(container, &pc_callbacks, load_data);
}

int spotifyctl_load_user_playlistcontainer(const char *user, void *reference, char **error_msg)
{
    sp_playlistcontainer *container = sp_session_publishedcontainer_for_user_create (g_state.session, user);
    if (!container) {
        *error_msg = "Not logged in";
        return CMD_RESULT_ERROR;
    }

    load_container(container, reference);

    return CMD_RESULT_OK;
}

int spotifyctl_load_playlistcontainer(void *reference, char **error_msg)
{
    if (!g_state.playlistcontainer) {
        *error_msg = "Not logged in";
        return CMD_RESULT_ERROR;
    }

    sp_playlistcontainer_add_ref(g_state.playlistcontainer);
    load_container(g_state.playlistcontainer, NULL);

    return CMD_RESULT_OK;
}

int spotifyctl_load_playlist(const char *link_str, void *reference, char **error_msg)
{
    sp_link *link;

    link = sp_link_create_from_string(link_str);
    CHECK_VALID_LINK(link);

    sp_playlist *playlist;
    playlist = sp_playlist_create(g_state.session, link);
    if (!playlist) {
        sp_link_release(link);
        *error_msg = "Link is not a playlist";
        return CMD_RESULT_ERROR;
    } 

    sp_playlist_add_ref(playlist);
    sp_playlist_add_callbacks(playlist, &load_playlist_callbacks, reference);

    if (sp_playlist_is_loaded(playlist)) {
        // call it myself
        pl_state_change(playlist, reference);
    }

    sp_link_release(link);

    return CMD_RESULT_OK;
}
