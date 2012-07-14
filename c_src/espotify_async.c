#include "erl_nif.h"
#include <string.h> /* strncpy */
#include <stdarg.h> /* va_* */
#include <stdio.h> /* fprintf */

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
    fprintf(stderr, "send to: %p\n", erl_pid);
    if (!enif_send(NULL, 
                   (ErlNifPid *)erl_pid, 
                   env,
                   enif_make_tuple3(
                       env,
                       enif_make_atom(env, "$spotify_callback"),
                       enif_make_atom(env, callback_name),
                       term)
            )) {
        fprintf(stderr, "error sending.... :-/\n");
    }
}

void esp_error_feedback(void *erl_pid, const char *callback_name, char *msg_in)
{
    ErlNifEnv* env = ensure_env();

    callback_result(erl_pid,
                    callback_name,
                    enif_make_tuple2(
                        env,
                        enif_make_atom(env, "error"),
                        enif_make_string(env, msg_in, ERL_NIF_LATIN1)
                        )
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

void esp_player_load_feedback(void *erl_pid) 
{
    ErlNifEnv* env = ensure_env();
    callback_result(erl_pid,
                    "player_load",
                    enif_make_atom(
                        env,
                        "loaded"
                        )
        );
    enif_clear_env(env);
}

