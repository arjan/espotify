/* espotify.c */
#include <unistd.h> /* sleep */
#include <stdio.h> /* printf */
#include <string.h> /* strcmp */

#include "spotifyctl/spotifyctl.h"
#include "erl_nif.h"

#define DBG(d) (fprintf(stderr, "DEBUG: " d "\n"))

#define ATOM_ERROR(env, s) (enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, s)))
#define STR_ERROR(env, s) (enif_make_tuple2(env, enif_make_atom(env, "error"), (s?enif_make_string(env, s, ERL_NIF_LATIN1):enif_make_atom(env, "unknown_error"))))

#define ASSERT_STARTED(priv) if (!priv->session) {return ATOM_ERROR(env, "not_started");}

#define MAX_PATH 1024
#define MAX_USERNAME 255
#define MAX_LINK 1024
#define MAX_ATOM 1024

typedef struct  {
    char username[MAX_USERNAME];
    char password[MAX_USERNAME];
    char cache_location[MAX_PATH];
    char settings_location[MAX_PATH];
    ErlNifPid pid; /* the calling process for sending async feedback. */
    ErlNifTid tid;
} espotify_session;

typedef struct
{
    espotify_session *session;
} espotify_private;

ErlNifPid return_pid;

void *run_main_thread(void *data)
{
    espotify_session *session = (espotify_session *)data;
    spotifyctl_run(&session->pid,
                   session->cache_location, session->settings_location,
                   session->username, session->password);
    return NULL;
}

static ERL_NIF_TERM espotify_start(ErlNifEnv* env, int argc, 
                                   const ERL_NIF_TERM argv[])
{
    espotify_private *priv = (espotify_private *)enif_priv_data(env);

    if (priv->session) {
        // Allow only one simultaneous session!
        return ATOM_ERROR(env, "already_started");
    }

    priv->session = (espotify_session *)enif_alloc(sizeof(espotify_session));

    if (!enif_get_local_pid(env, argv[0], &priv->session->pid))
        return enif_make_badarg(env);

    if (enif_get_string(env, argv[1], priv->session->cache_location, MAX_PATH, ERL_NIF_LATIN1) < 1)
        return enif_make_badarg(env);

    if (enif_get_string(env, argv[2], priv->session->settings_location, MAX_PATH, ERL_NIF_LATIN1) < 1)
        return enif_make_badarg(env);

    if (enif_get_string(env, argv[3], priv->session->username, MAX_USERNAME, ERL_NIF_LATIN1) < 1)
        return enif_make_badarg(env);

    if (enif_get_string(env, argv[4], priv->session->password, MAX_USERNAME, ERL_NIF_LATIN1) < 1)
        return enif_make_badarg(env);

    if (enif_thread_create("espotify main loop", &priv->session->tid, run_main_thread, (void *)priv->session, NULL))
        return enif_make_badarg(env);

    return enif_make_atom(env, "ok");
}


static ERL_NIF_TERM espotify_stop(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    espotify_private *priv = (espotify_private *)enif_priv_data(env);
    ASSERT_STARTED(priv);

    void *resp;
    char *error_msg;
    if (spotifyctl_do_cmd0(CMD_STOP, &error_msg))
        return enif_make_badarg(env);

    // wait for spotify thread to exit
    enif_thread_join(priv->session->tid, &resp);

    // clean up
    enif_free(priv->session);
    priv->session = 0;

    return enif_make_atom(env, "ok");
}


static ERL_NIF_TERM espotify_set_pid(ErlNifEnv* env, int argc, 
                                   const ERL_NIF_TERM argv[])
{
    espotify_private *priv = (espotify_private *)enif_priv_data(env);
    ASSERT_STARTED(priv);

    if (!enif_get_local_pid(env, argv[0], &priv->session->pid))
        return enif_make_badarg(env);

    spotifyctl_set_pid(&priv->session->pid);

    return enif_make_atom(env, "ok");
}

static ERL_NIF_TERM espotify_player_load(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    espotify_private *priv = (espotify_private *)enif_priv_data(env);
    ASSERT_STARTED(priv);

    char link[MAX_LINK];
    char *error_msg;

    if (!priv->session) {
        // No session started
        return ATOM_ERROR(env, "not_started");
    }

    if (enif_get_string(env, argv[0], link, MAX_LINK, ERL_NIF_LATIN1) < 1)
        return enif_make_badarg(env);

    switch (spotifyctl_do_cmd1(CMD_PLAYER_LOAD, (void *)link, &error_msg)) {
    case CMD_RESULT_ERROR:
        return STR_ERROR(env, error_msg);
    case CMD_RESULT_LOADING:
        return enif_make_atom(env, "loading");
    }
    return enif_make_atom(env, "ok");
}

static ERL_NIF_TERM espotify_player_prefetch(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    espotify_private *priv = (espotify_private *)enif_priv_data(env);
    ASSERT_STARTED(priv);

    return enif_make_atom(env, "error");
}

static ERL_NIF_TERM espotify_player_play(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    espotify_private *priv = (espotify_private *)enif_priv_data(env);
    ASSERT_STARTED(priv);

    char atom[MAX_ATOM];

   if (!enif_get_atom(env, argv[0], atom, MAX_ATOM, ERL_NIF_LATIN1))
        return enif_make_badarg(env);

    if (!spotifyctl_has_current_track())
        return ATOM_ERROR(env, "no_current_track");

    int play = strcmp(atom, "true") == 0;

    char *error_msg;
    spotifyctl_do_cmd1(CMD_PLAYER_PLAY, (void *)&play, &error_msg);
    return enif_make_atom(env, "ok");
}

static ERL_NIF_TERM espotify_player_seek(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    espotify_private *priv = (espotify_private *)enif_priv_data(env);
    ASSERT_STARTED(priv);

    unsigned int offset;
    if (!enif_get_uint(env, argv[0], &offset))
        return enif_make_badarg(env);

    if (!spotifyctl_has_current_track())
        return ATOM_ERROR(env, "no_current_track");

    char *error_msg;
    spotifyctl_do_cmd1(CMD_PLAYER_SEEK, (void *)&offset, &error_msg);
    return enif_make_atom(env, "ok");
}

static ERL_NIF_TERM espotify_player_unload(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    espotify_private *priv = (espotify_private *)enif_priv_data(env);
    ASSERT_STARTED(priv);

    if (!spotifyctl_has_current_track())
        return ATOM_ERROR(env, "no_current_track");

    char *error_msg;
    spotifyctl_do_cmd0(CMD_PLAYER_UNLOAD, &error_msg);
    return enif_make_atom(env, "ok");
}

static int load(ErlNifEnv* env, void** priv_data, ERL_NIF_TERM load_info)
{
    espotify_private* data = (espotify_private *)enif_alloc(sizeof(espotify_private));

    data->session = 0;
    
    *priv_data = data;
    return 0;
}

static void unload(ErlNifEnv* env, void* priv_data)
{
    enif_free(priv_data);
}

static ErlNifFunc nif_funcs[] =
{
    {"start", 5, espotify_start},
    {"stop", 0, espotify_stop},
    {"set_pid", 1, espotify_set_pid},
    {"player_load", 1, espotify_player_load},
    {"player_prefetch", 1, espotify_player_prefetch},
    {"player_play", 1, espotify_player_play},
    {"player_seek", 1, espotify_player_seek},
    {"player_unload", 0, espotify_player_unload}
};

ERL_NIF_INIT(espotify_nif,nif_funcs,load,NULL,NULL,unload)
