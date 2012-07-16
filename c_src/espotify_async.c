#include "erl_nif.h"
#include <string.h> /* strncpy */
#include <stdarg.h> /* va_* */
#include <stdio.h> /* fprintf */

#include "spotifyctl/spotifyctl.h"

#define DBG(d) (fprintf(stderr, "DEBUG: " d "\n"))

#define ERROR_TERM(env, term) enif_make_tuple2(env, enif_make_atom(env, "error"), term)
#define OK_TERM(env, term) enif_make_tuple2(env, enif_make_atom(env, "ok"), term)


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

void esp_logged_in_feedback(void *erl_pid, const char *link, const char *canonical_name, const char *display_name)
{
    ErlNifEnv* env = ensure_env();

    callback_result(erl_pid,
                    "logged_in",
                    enif_make_tuple2(
                        env,
                        enif_make_atom(env, "ok"),
                        enif_make_tuple4(
                            env,
                            enif_make_atom(env, "sp_user"),
                            enif_make_string(env, link, ERL_NIF_LATIN1),
                            enif_make_string(env, canonical_name, ERL_NIF_LATIN1),
                            enif_make_string(env, display_name, ERL_NIF_LATIN1)
                            )
                        )
        );
    enif_clear_env(env);
}


ERL_NIF_TERM track_tuple(spotifyctl_track *track)
{
    ErlNifEnv* env = ensure_env();
    return enif_make_tuple6(
        env,
        enif_make_atom(env, "sp_track"),
        enif_make_atom(env, track->is_loaded ? "true" : "false"),
        enif_make_atom(env, track->is_starred ? "true" : "false"),
        enif_make_atom(env, track->is_local ? "true" : "false"),
        enif_make_string(env, track->link, ERL_NIF_LATIN1),
        enif_make_string(env, track->track_name, ERL_NIF_LATIN1)
        );
}


void esp_player_load_feedback(void *erl_pid, spotifyctl_track *track) 
{
    ErlNifEnv* env = ensure_env();
    callback_result(erl_pid,
                    "player_load",
                    OK_TERM(env, track_tuple(track))
        );
    enif_clear_env(env);
}


void esp_player_track_feedback(void *erl_pid, spotifyctl_track *track)
{
    ErlNifEnv* env = ensure_env();
    callback_result(erl_pid,
                    "player_load",
                    OK_TERM(env, track_tuple(track))
        );
    enif_clear_env(env);
}
