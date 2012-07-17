#include "erl_nif.h"

#include <string.h> /* strncpy */
#include <stdarg.h> /* va_* */
#include <stdio.h> /* fprintf */

#include "spotifyctl/spotifyctl.h"
#include "espotify_util.h"

#define MAX_LINK 1024

#define DBG(d) (fprintf(stderr, "DEBUG: " d "\n"))


ErlNifEnv *ensure_env()
{
    static ErlNifEnv *env = 0;
    if (!env)
        env = enif_alloc_env();
    return env;
}

void esp_debug(void *erl_pid)
{
    ErlNifEnv* env = ensure_env();
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
    ErlNifEnv* env = ensure_env();
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
    ErlNifEnv* env = ensure_env();

    callback_result(erl_pid,
                    callback_name,
                    ERROR_TERM(env, enif_make_string(env, msg_in, ERL_NIF_LATIN1))
        );
    enif_clear_env(env);
}

void esp_atom_feedback(void *erl_pid, const char *callback_name, char *atom_in)
{
    ErlNifEnv* env = ensure_env();

    callback_result(erl_pid,
                    callback_name,
                    enif_make_atom(env, atom_in)
        );
    enif_clear_env(env);
}

void esp_logged_in_feedback(void *erl_pid, sp_session *sess, sp_user *user)
{
    ErlNifEnv* env = ensure_env();

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


ERL_NIF_TERM track_tuple(ErlNifEnv* env, sp_session *sess, sp_track *track)
{
    char link_str[MAX_LINK];

    sp_link *link = sp_link_create_from_track(track, 0);
    sp_link_as_string(link, link_str, MAX_LINK);
    sp_link_release(link);

    int loaded = sp_track_is_loaded(track);
    ERL_NIF_TERM undefined = enif_make_atom(env, "undefined");

    return enif_make_tuple(
        env,
        10,
        enif_make_atom(env, "sp_track"),
        enif_make_atom(env, loaded ? "true" : "false"),
        loaded ? BOOL_TERM(env, sp_track_is_starred(sess, track)) : undefined,
        loaded ? BOOL_TERM(env, sp_track_is_local(sess, track)) : undefined,
        enif_make_string(env, link_str, ERL_NIF_LATIN1),
        loaded ? enif_make_string(env, sp_track_name(track), ERL_NIF_LATIN1) : undefined,
        loaded ? enif_make_uint(env, sp_track_duration(track)) : undefined,
        loaded ? enif_make_uint(env, sp_track_popularity(track)) : undefined,
        loaded ? enif_make_uint(env, sp_track_disc(track)) : undefined,
        loaded ? enif_make_uint(env, sp_track_index(track)) : undefined
        );
}


void esp_player_load_feedback(void *erl_pid, sp_session *sess, sp_track *track) 
{
    ErlNifEnv* env = ensure_env();
    callback_result(erl_pid,
                    "player_load",
                    OK_TERM(env, track_tuple(env, sess, track))

        );
    enif_clear_env(env);
}

