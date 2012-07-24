#include "erl_nif.h"

#include <string.h> /* strncpy, memcpy */
#include <stdarg.h> /* va_* */
#include <stdio.h> /* fprintf */

#include "spotifyctl/spotifyctl.h"
#include "espotify_util.h"
#include "espotify_async.h"

#define DBG(x) fprintf(stderr, "DEBUG: " x "\n");
#define MAX_LINK 1024

static ErlNifEnv *_temp_env = NULL;

ErlNifEnv *temp_env()
{
    if (_temp_env == NULL)
        _temp_env = enif_alloc_env();
    return _temp_env;
}

void clean_temp_env()
{
    if (_temp_env) {
        enif_free_env(_temp_env);
        _temp_env = NULL;
    }
}

ERL_NIF_TERM make_binary(ErlNifEnv *env, const char *string)
{
    ErlNifBinary bin;
    enif_alloc_binary(strlen(string), &bin);
    memcpy(bin.data, string, strlen(string));
    return enif_make_binary(env, &bin);
}


ERL_NIF_TERM cb_result(ErlNifEnv *env, const char *callback_name, ERL_NIF_TERM term)
{
    return enif_make_tuple3(
        env,
        enif_make_atom(env, "$spotify_callback"),
        enif_make_atom(env, callback_name),
        term);
}

void callback_result(ErlNifPid *pid, const char *callback_name, ERL_NIF_TERM term)
{
}
/*
 * ============ Start data representation =============
 */

ERL_NIF_TERM image_tuple(ErlNifEnv* env, sp_session *sess, sp_image *image)
{
    ErlNifBinary id_bin;
    enif_alloc_binary(20, &id_bin);
    id_bin.size = 20;
    memcpy(id_bin.data, sp_image_image_id(image), 20);

    size_t data_sz;
    const void *image_data = sp_image_data(image, &data_sz);
    ErlNifBinary data_bin;
    enif_alloc_binary(data_sz, &data_bin);
    id_bin.size = data_sz;
    memcpy(data_bin.data, image_data, data_sz);

    return enif_make_tuple4(
        env,
        enif_make_atom(env, "sp_image"),
        enif_make_atom(env, sp_image_format(image) == SP_IMAGE_FORMAT_JPEG  ? "jpeg" : "unknown"),
        enif_make_binary(env, &data_bin),
        enif_make_binary(env, &id_bin)
        );
}

ERL_NIF_TERM image_link_from_image_id(ErlNifEnv* env, sp_session *sess, const byte image_id[20])
{
    sp_image *image = sp_image_create(sess, image_id);
    sp_link *link = sp_link_create_from_image(image);
    sp_image_release(image);
    char link_str[MAX_LINK];
    sp_link_as_string(link, link_str, MAX_LINK);
    sp_link_release(link);
    return enif_make_string(env, link_str, ERL_NIF_LATIN1);
}

ERL_NIF_TERM artist_tuple(ErlNifEnv* env, sp_session *sess, sp_artist *artist)
{
    char link_str[MAX_LINK];
    char portrait_link_str[MAX_LINK];
    sp_link *link;
    int has_portrait = 0;

    link = sp_link_create_from_artist(artist);
    sp_link_as_string(link, link_str, MAX_LINK);
    sp_link_release(link);

    int loaded = sp_artist_is_loaded(artist);

    if (loaded) {
        link = sp_link_create_from_artist_portrait(artist, SP_IMAGE_SIZE_NORMAL);
        if (link) {
            has_portrait = 1;
            sp_link_as_string(link, portrait_link_str, MAX_LINK);
            sp_link_release(link);
        }
    }

    ERL_NIF_TERM undefined = enif_make_atom(env, "undefined");

    return enif_make_tuple(
        env,
        5,
        enif_make_atom(env, "sp_artist"),
        BOOL_TERM(env, loaded),
        enif_make_string(env, link_str, ERL_NIF_LATIN1),
        loaded ? make_binary(env, sp_artist_name(artist)) : undefined,
        (loaded && has_portrait) ? enif_make_string(env, portrait_link_str, ERL_NIF_LATIN1) : undefined
        );
}


ERL_NIF_TERM album_tuple(ErlNifEnv* env, sp_session *sess, sp_album *album, int recurse)
{
    char link_str[MAX_LINK];

    sp_link *link = sp_link_create_from_album(album);
    sp_link_as_string(link, link_str, MAX_LINK);
    sp_link_release(link);

    int loaded = sp_album_is_loaded(album);
    ERL_NIF_TERM undefined = enif_make_atom(env, "undefined");

    char *type_str = "undefined";
    switch (sp_album_type(album))
    {
    case SP_ALBUMTYPE_ALBUM:
        type_str = "album"; break;
    case SP_ALBUMTYPE_SINGLE:
        type_str = "single"; break;
    case SP_ALBUMTYPE_COMPILATION:
        type_str = "compilation"; break;
    case SP_ALBUMTYPE_UNKNOWN:
        type_str = "unknown"; break;
    }

    return enif_make_tuple(
        env,
        9,
        enif_make_atom(env, "sp_album"),
        BOOL_TERM(env, loaded),
        enif_make_string(env, link_str, ERL_NIF_LATIN1),
        loaded ? BOOL_TERM(env, sp_album_is_available(album)) : undefined,
        loaded && recurse ? artist_tuple(env, sess, sp_album_artist(album)) : undefined,
        undefined, // FIXME cover loaded ? make_binary(env, sp_album_cover(album)) : undefined,
        loaded ? make_binary(env, sp_album_name(album)) : undefined,
        loaded ? enif_make_uint(env, sp_album_year(album)) : undefined,
        enif_make_atom(env, type_str)
        );
}

ERL_NIF_TERM track_tuple(ErlNifEnv* env, sp_session *sess, sp_track *track, int recurse)
{
    char link_str[MAX_LINK];

    sp_link *link = sp_link_create_from_track(track, 0);
    sp_link_as_string(link, link_str, MAX_LINK);
    sp_link_release(link);

    int loaded = sp_track_is_loaded(track);
    ERL_NIF_TERM undefined = enif_make_atom(env, "undefined");

    ERL_NIF_TERM artists = undefined;
    if (loaded && recurse) {
        int total = sp_track_num_artists(track);
        int i;
        ERL_NIF_TERM *as = (ERL_NIF_TERM *)enif_alloc(total * sizeof(ERL_NIF_TERM));
        for (i=0; i<total; i++) {
            as[i] = artist_tuple(env, sess, sp_track_artist(track, i));
        }
        artists = enif_make_list_from_array(env, as, total);
        enif_free(as);
    }

    return enif_make_tuple(
        env,
        12,
        enif_make_atom(env, "sp_track"),
        BOOL_TERM(env, loaded),
        enif_make_string(env, link_str, ERL_NIF_LATIN1),
        loaded ? BOOL_TERM(env, sp_track_is_starred(sess, track)) : undefined,
        loaded ? BOOL_TERM(env, sp_track_is_local(sess, track)) : undefined,
        artists,
        loaded && recurse ? album_tuple(env, sess, sp_track_album(track), 0) : undefined,
        loaded ? make_binary(env, sp_track_name(track)) : undefined,
        loaded ? enif_make_uint(env, sp_track_duration(track)) : undefined,
        loaded ? enif_make_uint(env, sp_track_popularity(track)) : undefined,
        loaded ? enif_make_uint(env, sp_track_disc(track)) : undefined,
        loaded ? enif_make_uint(env, sp_track_index(track)) : undefined
        );
}

ERL_NIF_TERM albumbrowse_tuple(ErlNifEnv* env, sp_session *sess, sp_albumbrowse *albumbrowse)
{
    int total, i;
    ERL_NIF_TERM *list;

    // tracks 
    total = sp_albumbrowse_num_tracks(albumbrowse);
    list = (ERL_NIF_TERM *)enif_alloc(total * sizeof(ERL_NIF_TERM));
    for (i=0; i<total; i++) {
        list[i] = track_tuple(env, sess, sp_albumbrowse_track(albumbrowse, i), 0);
    }
    ERL_NIF_TERM tracks = enif_make_list_from_array(env, list, total);
    enif_free(list);

    // copyrights
    total = sp_albumbrowse_num_copyrights(albumbrowse);
    list = (ERL_NIF_TERM *)enif_alloc(total * sizeof(ERL_NIF_TERM));
    for (i=0; i<total; i++) {
        list[i] = make_binary(env, sp_albumbrowse_copyright(albumbrowse, i));
    }
    ERL_NIF_TERM copyrights = enif_make_list_from_array(env, list, total);
    enif_free(list);

    return enif_make_tuple(
        env,
        6,
        enif_make_atom(env, "sp_albumbrowse"),
        album_tuple(env, sess, sp_albumbrowse_album(albumbrowse), 0),
        artist_tuple(env, sess, sp_albumbrowse_artist(albumbrowse)),
        copyrights,
        tracks,
        make_binary(env, sp_albumbrowse_review(albumbrowse))
        );
}

ERL_NIF_TERM artistbrowse_tuple(ErlNifEnv* env, sp_session *sess, sp_artistbrowse *artistbrowse)
{
    int total, i;
    ERL_NIF_TERM *list;

    // list: tracks
    total = sp_artistbrowse_num_tracks(artistbrowse);
    list = (ERL_NIF_TERM *)enif_alloc(total * sizeof(ERL_NIF_TERM));
    for (i=0; i<total; i++) {
        list[i] = track_tuple(env, sess, sp_artistbrowse_track(artistbrowse, i), 0);
    }
    ERL_NIF_TERM tracks = enif_make_list_from_array(env, list, total);
    enif_free(list);

    // list: albums
    total = sp_artistbrowse_num_albums(artistbrowse);
    list = (ERL_NIF_TERM *)enif_alloc(total * sizeof(ERL_NIF_TERM));
    for (i=0; i<total; i++) {
        list[i] = album_tuple(env, sess, sp_artistbrowse_album(artistbrowse, i), 0);
    }
    ERL_NIF_TERM albums = enif_make_list_from_array(env, list, total);
    enif_free(list);

    // list: tophit_tracks
    total = sp_artistbrowse_num_tophit_tracks(artistbrowse);
    list = (ERL_NIF_TERM *)enif_alloc(total * sizeof(ERL_NIF_TERM));
    for (i=0; i<total; i++) {
        list[i] = track_tuple(env, sess, sp_artistbrowse_tophit_track(artistbrowse, i), 0);
    }
    ERL_NIF_TERM tophit_tracks = enif_make_list_from_array(env, list, total);
    enif_free(list);

    // list: simiar_artists
    total = sp_artistbrowse_num_similar_artists(artistbrowse);
    list = (ERL_NIF_TERM *)enif_alloc(total * sizeof(ERL_NIF_TERM));
    for (i=0; i<total; i++) {
        list[i] = artist_tuple(env, sess, sp_artistbrowse_similar_artist(artistbrowse, i));
    }
    ERL_NIF_TERM similar_artists = enif_make_list_from_array(env, list, total);
    enif_free(list);

    // list: portraits
    total = sp_artistbrowse_num_portraits(artistbrowse);
    list = (ERL_NIF_TERM *)enif_alloc(total * sizeof(ERL_NIF_TERM));
    for (i=0; i<total; i++) {
        sp_link *link = sp_link_create_from_artistbrowse_portrait(artistbrowse, i);
        char link_str[MAX_LINK];
        sp_link_as_string(link, link_str, MAX_LINK);
        sp_link_release(link);
        list[i] = enif_make_string(env, link_str, ERL_NIF_LATIN1);
    }
    ERL_NIF_TERM portraits = enif_make_list_from_array(env, list, total);
    enif_free(list);

    return enif_make_tuple(
        env,
        8,
        enif_make_atom(env, "sp_artistbrowse"),
        artist_tuple(env, sess, sp_artistbrowse_artist(artistbrowse)),
        portraits,
        tracks,
        tophit_tracks,
        albums,
        similar_artists,
        make_binary(env, sp_artistbrowse_biography(artistbrowse))
        );
}


ERL_NIF_TERM user_tuple(ErlNifEnv* env, sp_session *sess, sp_user *user)
{
    // Make login feedback
    char link_str[MAX_LINK];
    sp_link *link = sp_link_create_from_user(user);
    sp_link_as_string(link, link_str, MAX_LINK);
    sp_link_release(link);

    return enif_make_tuple(
        env,
        4,
        enif_make_atom(env, "sp_user"),
        enif_make_string(env, link_str, ERL_NIF_LATIN1),
        make_binary(env, sp_user_canonical_name(user)),
        make_binary(env, sp_user_display_name(user))
        );
}

ERL_NIF_TERM playlist_track_tuple(ErlNifEnv* env, sp_session *sess, sp_playlist *playlist, int track, int recurse)
{
    ERL_NIF_TERM undefined = enif_make_atom(env, "undefined");
    const char *message = sp_playlist_track_message(playlist, track);
    return enif_make_tuple(
        env,
        6,
        enif_make_atom(env, "sp_playlist_track"),
        track_tuple(env, sess, sp_playlist_track(playlist, track), 1),
        enif_make_uint(env, sp_playlist_track_create_time(playlist, track)),
        user_tuple(env, sess, sp_playlist_track_creator(playlist, track)),
        BOOL_TERM(env, sp_playlist_track_seen(playlist, track)),
        message ? make_binary(env, message) : undefined
        );
}

ERL_NIF_TERM playlist_tuple(ErlNifEnv* env, sp_session *sess, sp_playlist *playlist, int recurse)
{
    // Make login feedback
    char link_str[MAX_LINK];
    sp_link *link = sp_link_create_from_playlist(playlist);
    sp_link_as_string(link, link_str, MAX_LINK);
    sp_link_release(link);

    ERL_NIF_TERM undefined = enif_make_atom(env, "undefined");
    int loaded = sp_playlist_is_loaded(playlist);

    byte image_id[20];
    ERL_NIF_TERM image;
    if (sp_playlist_get_image(playlist, image_id)) {
        image = image_link_from_image_id(env, sess, image_id);
    } else {
        image = undefined;
    }

    int total, i;
    ERL_NIF_TERM *list;

    ERL_NIF_TERM num_tracks = undefined;
    if (loaded) {
        num_tracks = enif_make_uint(env, sp_playlist_num_tracks(playlist));
    }

    ERL_NIF_TERM tracks = undefined;
    if (loaded && recurse) {
        // list: tracks
        total = sp_playlist_num_tracks(playlist);
        list = (ERL_NIF_TERM *)enif_alloc(total * sizeof(ERL_NIF_TERM));
        for (i=0; i<total; i++) {
            list[i] = playlist_track_tuple(env, sess, playlist, i, 1);
        }
        tracks = enif_make_list_from_array(env, list, total);
        enif_free(list);
    }

    const char *description = sp_playlist_get_description(playlist);

    return enif_make_tuple(
        env,
        10,
        enif_make_atom(env, "sp_playlist"),
        BOOL_TERM(env, loaded),
        enif_make_string(env, link_str, ERL_NIF_LATIN1),
        make_binary(env, sp_playlist_name(playlist)),
        user_tuple(env, sess, sp_playlist_owner(playlist)),
        BOOL_TERM(env, sp_playlist_is_collaborative(playlist)),
        description ? make_binary(env, description) : undefined,
        image,
        num_tracks,
        tracks
        );
}

ERL_NIF_TERM playlistcontainer_tuple(ErlNifEnv* env, sp_session *sess, sp_playlistcontainer *container)
{
    char foldername[MAX_LINK];

    int total, i;
    ERL_NIF_TERM *list;
    sp_playlist *pl;
    sp_link *link;

    total = sp_playlistcontainer_num_playlists(container);
    list = (ERL_NIF_TERM *)enif_alloc(total * sizeof(ERL_NIF_TERM));
    for (i=0; i<total; i++) {
        switch (sp_playlistcontainer_playlist_type(container, i)) {
        case SP_PLAYLIST_TYPE_PLAYLIST:
            pl = sp_playlistcontainer_playlist(container, i);
            link = sp_link_create_from_playlist(pl);
            if (link == NULL) {
                // Not loaded...
                list[i] = enif_make_atom(env, "not_loaded");
            } else {
                sp_link_release(link);
                list[i] = playlist_tuple(env, sess, pl, 0);
            }
            break;
        case SP_PLAYLIST_TYPE_START_FOLDER:
            sp_playlistcontainer_playlist_folder_name(container, i, foldername, MAX_LINK),
            list[i] = enif_make_tuple3(
                env, 
                enif_make_atom(env, "start_folder"),
                enif_make_uint64(env, 
                                 sp_playlistcontainer_playlist_folder_id(container, i)),
                make_binary(env, 
                            foldername)
                );
            break;
        case SP_PLAYLIST_TYPE_END_FOLDER:
            list[i] = enif_make_atom(env, "end_folder");
            break;
        case SP_PLAYLIST_TYPE_PLACEHOLDER:
            list[i] = enif_make_atom(env, "placeholder");
            break;
        }
    }
    ERL_NIF_TERM contents = enif_make_list_from_array(env, list, total);
    enif_free(list);

    return enif_make_tuple(
        env,
        3,
        enif_make_atom(env, "sp_playlistcontainer"),
        user_tuple(env, sess, sp_playlistcontainer_owner(container)),
        contents);
}


ERL_NIF_TERM search_result_tuple(ErlNifEnv* env, sp_session *sess, sp_search *search)
{
    int total, i;
    ERL_NIF_TERM *list;

    // list: tracks
    total = sp_search_num_tracks(search);
    list = (ERL_NIF_TERM *)enif_alloc(total * sizeof(ERL_NIF_TERM));
    for (i=0; i<total; i++) {
        list[i] = track_tuple(env, sess, sp_search_track(search, i), 1);
    }
    ERL_NIF_TERM tracks = enif_make_list_from_array(env, list, total);
    enif_free(list);

    // list: albums
    total = sp_search_num_albums(search);
    list = (ERL_NIF_TERM *)enif_alloc(total * sizeof(ERL_NIF_TERM));
    for (i=0; i<total; i++) {
        list[i] = album_tuple(env, sess, sp_search_album(search, i), 1);
    }
    ERL_NIF_TERM albums = enif_make_list_from_array(env, list, total);
    enif_free(list);

    // list: artists
    total = sp_search_num_artists(search);
    list = (ERL_NIF_TERM *)enif_alloc(total * sizeof(ERL_NIF_TERM));
    for (i=0; i<total; i++) {
        list[i] = artist_tuple(env, sess, sp_search_artist(search, i));
    }
    ERL_NIF_TERM artists = enif_make_list_from_array(env, list, total);
    enif_free(list);

    // list: playlists
    total = sp_search_num_playlists(search);
    list = (ERL_NIF_TERM *)enif_alloc(total * sizeof(ERL_NIF_TERM));
    for (i=0; i<total; i++) {
        list[i] = playlist_tuple(env, sess, sp_search_playlist(search, i), 0);
    }
    ERL_NIF_TERM playlists = enif_make_list_from_array(env, list, total);
    enif_free(list);


    return enif_make_tuple(
        env,
        11,
        enif_make_atom(env, "sp_search_result"),
        make_binary(env, sp_search_query(search)), // q :: string(),
        make_binary(env, sp_search_did_you_mean(search)), // did_you_mean :: string(),
        enif_make_uint(env, sp_search_total_tracks(search)), // total_tracks :: non_neg_integer(),
        enif_make_uint(env, sp_search_total_albums(search)), // total_albums :: non_neg_integer(),
        enif_make_uint(env, sp_search_total_artists(search)), // total_artists :: non_neg_integer(),
        enif_make_uint(env, sp_search_total_playlists(search)), // total_playlists :: non_neg_integer(),
        tracks, 
        albums, 
        artists,
        playlists
        );
}

/*
 * ============ Start feedback =============
 */

ERL_NIF_TERM *obtain_reference(ErlNifEnv *creation_env)
{
    ERL_NIF_TERM *reference = (ERL_NIF_TERM *)enif_alloc(sizeof(ERL_NIF_TERM));
    *reference = enif_make_ref(creation_env);
    return reference;
}

ERL_NIF_TERM return_reference(ERL_NIF_TERM *refptr)
{
    // Copy the reference to the enif_send temp environment
    ERL_NIF_TERM ref_term = enif_make_copy(temp_env(), *refptr);
    enif_free(refptr);
    return ref_term;
}

void esp_error_feedback(void *erl_pid, const char *callback_name, char *msg_in)
{
    ErlNifEnv* env = temp_env();

    callback_result(erl_pid,
                    callback_name,
                    ERROR_TERM(env, make_binary(env, msg_in))
        );
    enif_clear_env(env);
}

void esp_atom_feedback(void *erl_pid, const char *callback_name, char *atom_in)
{
    ErlNifEnv* env = temp_env();

    callback_result(erl_pid,
                    callback_name,
                    enif_make_atom(env, atom_in)
        );
    enif_clear_env(env);
}

void esp_logged_in_feedback(void *state, sp_session *sess, sp_user *user)
{
    ErlNifEnv* env = async_env_acquire((async_state_t *)state);
    async_env_release_and_send(
        (async_state_t *)state,
        cb_result(env,
                  "logged_in",
                  enif_make_tuple2(
                      env,
                      enif_make_atom(env, "ok"),
                      user_tuple(env, sess, user)
                      )
            ));
}

void esp_player_load_feedback(void *erl_pid, sp_session *sess, sp_track *track) 
{
    ErlNifEnv* env = temp_env();
    callback_result(erl_pid,
                    "player_load",
                    OK_TERM(env, track_tuple(env, sess, track, 1))

        );
    enif_clear_env(env);
}

void esp_player_track_info_feedback(void *erl_pid, sp_session *sess, void *refptr, sp_track *track)
{
    ErlNifEnv* env = temp_env();

    callback_result(erl_pid,
                    "track_info",
                    OK_TERM(env,
                            enif_make_tuple2(
                                env,
                                return_reference((ERL_NIF_TERM *)refptr),
                                track_tuple(env, sess, track, 1))
                        )
        );
    enif_clear_env(env);
}

void esp_player_browse_album_feedback(void *erl_pid, sp_session *session, void *refptr, sp_albumbrowse *albumbrowse)
{
    ErlNifEnv* env = temp_env();
        
    callback_result(erl_pid,
                    "browse_album",
                    OK_TERM(env,
                            enif_make_tuple2(
                                env,
                                return_reference((ERL_NIF_TERM *)refptr),
                                albumbrowse_tuple(env, session, albumbrowse))
                        )
        );
    enif_clear_env(env);
}

void esp_player_browse_artist_feedback(void *erl_pid, sp_session *session, void *refptr, sp_artistbrowse *artistbrowse)
{
    ErlNifEnv* env = temp_env();
        
    callback_result(erl_pid,
                    "browse_artist",
                    OK_TERM(env,
                            enif_make_tuple2(
                                env,
                                return_reference((ERL_NIF_TERM *)refptr),
                                artistbrowse_tuple(env, session, artistbrowse))
                        )
        );
    enif_clear_env(env);
}

void esp_player_load_image_feedback(void *erl_pid, sp_session *session, void *refptr, sp_image *image)
{
    ErlNifEnv* env = temp_env();
        
    callback_result(erl_pid,
                    "load_image",
                    OK_TERM(env,
                            enif_make_tuple2(
                                env,
                                return_reference((ERL_NIF_TERM *)refptr),
                                image_tuple(env, session, image))
                        )
        );
    enif_clear_env(env);
}


void esp_player_search_feedback(void *erl_pid, sp_session *session, void *refptr, sp_search *search)
{
    ErlNifEnv* env = temp_env();
        
    callback_result(erl_pid,
                    "search",
                    OK_TERM(env,
                            enif_make_tuple2(
                                env,
                                return_reference((ERL_NIF_TERM *)refptr),
                                search_result_tuple(env, session, search))
                        )
        );
    enif_clear_env(env);
}


void esp_player_load_playlistcontainer_feedback(void *erl_pid, sp_session *session, void *refptr, sp_playlistcontainer *container)
{
    ErlNifEnv* env = temp_env();
    ERL_NIF_TERM ref;
    if (refptr) {
        ref = return_reference((ERL_NIF_TERM *)refptr);
    } else {
        ref = enif_make_atom(env, "undefined");
    }
    callback_result(erl_pid,
                    "load_playlistcontainer",
                    OK_TERM(env,
                            enif_make_tuple2(
                                env,
                                ref,
                                playlistcontainer_tuple(env, session, container))
                        )
        );
    enif_clear_env(env);
}

void esp_player_load_playlist_feedback(void *erl_pid, sp_session *session, void *refptr, sp_playlist *playlist)
{
    ErlNifEnv* env = temp_env();
        
    callback_result(erl_pid,
                    "load_playlist",
                    OK_TERM(env,
                            enif_make_tuple2(
                                env,
                                return_reference((ERL_NIF_TERM *)refptr),
                                playlist_tuple(env, session, playlist, 1))
                        )
        );
    enif_clear_env(env);
}


void esp_debug(void *st, void *refptr)
{
    async_state_t *state = (async_state_t *)st;
    ErlNifEnv* env = async_env_acquire(state);
    async_env_release_and_send(
        state,
        cb_result(
            env,
            "debug",
            enif_make_atom(env, "ok..")
            /* OK_TERM(env, */
            /*         async_return_ref(state, (ERL_NIF_TERM *)refptr) */
            /*     ) */
            /* ) */
            ));
}
