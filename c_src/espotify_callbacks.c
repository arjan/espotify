#include <stdio.h> /* fprintf */

#include "espotify_util.h"
#include "espotify_async.h"

#define DBG(x) fprintf(stderr, "DEBUG: " x "\n");


ERL_NIF_TERM cb_result(ErlNifEnv *env, const char *callback_name, ERL_NIF_TERM term)
{
    return enif_make_tuple3(
        env,
        make_atom(env, "$spotify_callback"),
        make_atom(env, callback_name),
        term);
}

/*
 * ============ Start feedback =============
 */

void esp_error_feedback(void *st, const char *callback_name, char *msg_in)
{
    async_state_t *state = (async_state_t *)st;
    ErlNifEnv* env = async_env_acquire(state);

    async_env_release_and_send(
        state, env, 
        cb_result(
            env,
            callback_name,
            ERROR_TERM(env, make_binary(env, msg_in))
            ));
}

void esp_atom_feedback(void *st, const char *callback_name, char *atom_in)
{
    async_state_t *state = (async_state_t *)st;
    ErlNifEnv* env = async_env_acquire(state);

    async_env_release_and_send(
        state, env, 
        cb_result(
            env,
            callback_name,
            make_atom(env, atom_in)
            ));
}

void esp_logged_in_feedback(void *state, sp_session *sess, sp_user *user)
{
    ErlNifEnv* env = async_env_acquire((async_state_t *)state);
    async_env_release_and_send(
        (async_state_t *)state, env, 
        cb_result(env,
                  "logged_in",
                  enif_make_tuple2(
                      env,
                      make_atom(env, "ok"),
                      user_tuple(env, sess, user)
                      )
            ));
}

void esp_player_load_feedback(void *st, sp_session *sess, sp_track *track) 
{
    async_state_t *state = (async_state_t *)st;
    ErlNifEnv* env = async_env_acquire(state);

    async_env_release_and_send(
        state, env, 
        cb_result(
            env,
            "player_load",
            OK_TERM(env, track_tuple(env, sess, track, 1))
            ));
}

void esp_player_track_info_feedback(void *st, sp_session *sess, void *refptr, sp_track *track)
{
    async_state_t *state = (async_state_t *)st;
    ErlNifEnv* env = async_env_acquire(state);

    async_env_release_and_send(
        state, env, 
        cb_result(
            env,
            "track_info",
            OK_TERM(env,
                    enif_make_tuple2(
                        env,
                        async_return_ref(state, (ERL_NIF_TERM *)refptr),
                        track_tuple(env, sess, track, 1))
                )
            ));
}

void esp_player_browse_album_feedback(void *st, sp_session *session, void *refptr, sp_albumbrowse *albumbrowse)
{
    async_state_t *state = (async_state_t *)st;
    ErlNifEnv* env = async_env_acquire(state);

    async_env_release_and_send(
        state, env, 
        cb_result(
            env,
            "browse_album",
            OK_TERM(env,
                    enif_make_tuple2(
                        env,
                        async_return_ref(state, (ERL_NIF_TERM *)refptr),
                        albumbrowse_tuple(env, session, albumbrowse))
                )
            ));
}

void esp_player_browse_artist_feedback(void *st, sp_session *session, void *refptr, sp_artistbrowse *artistbrowse)
{
    async_state_t *state = (async_state_t *)st;
    ErlNifEnv* env = async_env_acquire(state);

    async_env_release_and_send(
        state, env, 
        cb_result(
            env,
            "browse_artist",
            OK_TERM(env,
                    enif_make_tuple2(
                        env,
                        async_return_ref(state, (ERL_NIF_TERM *)refptr),
                        artistbrowse_tuple(env, session, artistbrowse))
                )
            ));
}

void esp_player_load_image_feedback(void *st, sp_session *session, void *refptr, sp_image *image)
{
    async_state_t *state = (async_state_t *)st;
    ErlNifEnv* env = async_env_acquire(state);

    async_env_release_and_send(
        state, env, 
        cb_result(
            env,
            "load_image",
            OK_TERM(env,
                    enif_make_tuple2(
                        env,
                        async_return_ref(state, (ERL_NIF_TERM *)refptr),
                        image_tuple(env, session, image))
                )
            ));
}


void esp_player_search_feedback(void *st, sp_session *session, void *refptr, sp_search *search)
{
    async_state_t *state = (async_state_t *)st;
    ErlNifEnv* env = async_env_acquire(state);

    async_env_release_and_send(
        state, env, 
        cb_result(
            env,
            "search",
            OK_TERM(env,
                    enif_make_tuple2(
                        env,
                        async_return_ref(state, (ERL_NIF_TERM *)refptr),
                        search_result_tuple(env, session, search))
                        )
            ));
}


void esp_player_load_playlistcontainer_feedback(void *st, sp_session *session, void *refptr, sp_playlistcontainer *container)
{
    DBG("PL container load");

    async_state_t *state = (async_state_t *)st;
    ErlNifEnv* env = async_env_acquire(state);

    ERL_NIF_TERM ref;
    if (refptr) {
        ref = async_return_ref(state,  (ERL_NIF_TERM *)refptr);
    } else {
        ref = make_atom(env, "undefined");
    }

    async_env_release_and_send(
        state, env, 
        cb_result(
            env,
            "load_playlistcontainer",
            OK_TERM(env,
                    enif_make_tuple2(
                        env,
                        ref,
                        playlistcontainer_tuple(env, session, container)
                        )
                )
            )
        );
}

void esp_player_load_playlist_feedback(void *st, sp_session *session, void *refptr, sp_playlist *playlist)
{
    async_state_t *state = (async_state_t *)st;
    ErlNifEnv* env = async_env_acquire(state);
    
    async_env_release_and_send(
        state, env, 
        cb_result(
            env,
            "load_playlist",
            OK_TERM(env,
                    enif_make_tuple2(
                        env,
                        async_return_ref(state, (ERL_NIF_TERM *)refptr),
                        playlist_tuple(env, session, playlist, 1))
                )
            ));
}

void esp_debug(void *st, void *refptr)
{
    async_state_t *state = (async_state_t *)st;
    ErlNifEnv* env = async_env_acquire(state);

    async_env_release_and_send(
        state, env, 
        cb_result(
            env,
            "debug",
            OK_TERM(env,
                    async_return_ref(state, (ERL_NIF_TERM *)refptr)
                )
            )
        );
}
