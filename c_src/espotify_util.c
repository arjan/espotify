#include "erl_nif.h"

#include <string.h> /* strncpy, memcpy */
#include <stdarg.h> /* va_* */
#include <stdio.h> /* fprintf */

#include "spotifyctl/spotifyctl.h"
#include "espotify_util.h"

#define MAX_LINK 1024

#define DBG(d) (fprintf(stderr, "DEBUG: " d "\n"))


ErlNifEnv *temp_env()
{
    static ErlNifEnv *env = 0;
    if (!env)
        env = enif_alloc_env();
    return env;
}


void esp_debug(void *erl_pid)
{
    ErlNifEnv* env = temp_env();
    fprintf(stderr, "debug send to: %p\n", erl_pid);
    if (!enif_send(NULL, 
                   (ErlNifPid *)erl_pid, 
                   env,
                   enif_make_atom(env, "debug")))
    {
        fprintf(stderr, "error sending debug.... :-/\n");
    }
}

void callback_result(void *erl_pid, const char *callback_name, ERL_NIF_TERM term)
{
    ErlNifEnv* env = temp_env();
    if (!enif_send(NULL, 
                   (ErlNifPid *)erl_pid, 
                   env,
                   enif_make_tuple3(
                       env,
                       enif_make_atom(env, "$spotify_callback"),
                       enif_make_atom(env, callback_name),
                       term)
            )) {
        fprintf(stderr, "error sending feedback, process might be gone\n");
    }
}

void esp_error_feedback(void *erl_pid, const char *callback_name, char *msg_in)
{
    ErlNifEnv* env = temp_env();

    callback_result(erl_pid,
                    callback_name,
                    ERROR_TERM(env, enif_make_string(env, msg_in, ERL_NIF_LATIN1))
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

void esp_logged_in_feedback(void *erl_pid, sp_session *sess, sp_user *user)
{
    ErlNifEnv* env = temp_env();

    // Make login feedback
    char link_str[MAX_LINK];
    sp_link *link = sp_link_create_from_user(user);
    sp_link_as_string(link, link_str, MAX_LINK);
    sp_link_release(link);
    
    callback_result(erl_pid,
                    "logged_in",
                    enif_make_tuple2(
                        env,
                        enif_make_atom(env, "ok"),
                        enif_make_tuple4(
                            env,
                            enif_make_atom(env, "sp_user"),
                            enif_make_string(env, link_str, ERL_NIF_LATIN1),
                            enif_make_string(env, sp_user_canonical_name(user), ERL_NIF_LATIN1),
                            enif_make_string(env, sp_user_display_name(user), ERL_NIF_LATIN1)
                            )
                        )
        );
    enif_clear_env(env);
}


ERL_NIF_TERM image_tuple(ErlNifEnv* env, sp_session *sess, sp_image *image)
{
    ErlNifBinary id_bin;
    enif_alloc_binary(20, &id_bin);
    id_bin.size = 20;
    memcpy(id_bin.data, sp_image_image_id(image), 20);

    size_t data_sz;
    void *image_data = sp_image_data(image, &data_sz);
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
        loaded ? enif_make_string(env, sp_artist_name(artist), ERL_NIF_LATIN1) : undefined,
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
        undefined, // FIXME cover loaded ? enif_make_string(env, sp_album_cover(album), ERL_NIF_LATIN1) : undefined,
        loaded ? enif_make_string(env, sp_album_name(album), ERL_NIF_LATIN1) : undefined,
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
        loaded ? enif_make_string(env, sp_track_name(track), ERL_NIF_LATIN1) : undefined,
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
        list[i] = enif_make_string(env, sp_albumbrowse_copyright(albumbrowse, i), ERL_NIF_LATIN1);
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
        enif_make_string(env, sp_albumbrowse_review(albumbrowse), ERL_NIF_LATIN1)
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
        enif_make_string(env, sp_artistbrowse_biography(artistbrowse), ERL_NIF_LATIN1)
        );
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


ERL_NIF_TERM *obtain_reference(ErlNifEnv *creation_env)
{
    ERL_NIF_TERM *reference = (ERL_NIF_TERM *)enif_alloc(sizeof(ERL_NIF_TERM));
    *reference = enif_make_ref(creation_env);
    return reference;
}

ERL_NIF_TERM return_reference(ERL_NIF_TERM *refptr)
{
    // Copy the reference to the enif_send temp environment
    ERL_NIF_TERM *ref = (ERL_NIF_TERM *)refptr;
    ERL_NIF_TERM ref_term = enif_make_copy(temp_env(), *ref);
    enif_free(refptr);
    return ref_term;
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


void esp_player_playlist_container_feedback(void *erl_pid, sp_session *session, void *refptr, sp_playlistcontainer *container)
{
    ErlNifEnv* env = temp_env();

    /* callback_result(erl_pid, */
    /*                 "browse_album", */
    /*                 OK_TERM(env, */
    /*                         enif_make_tuple2( */
    /*                             env, */
    /*                             ref_term, */
    /*                             albumbrowse_tuple(env, session, albumbrowse)) */
    /*                     ) */
    /*     ); */
    enif_clear_env(env);
}
