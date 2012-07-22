/* espotify.c */
#include <unistd.h> /* sleep */
#include <stdio.h> /* printf */
#include <string.h> /* strcmp */

#include "spotifyctl/spotifyctl.h"
#include "espotify_util.h"

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
    ErlNifEnv *callback_env;
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

    spotifyctl_stop();

    // wait for spotify thread to exit
    enif_thread_join(priv->session->tid, &resp);

    // clean up
    enif_free(priv->session);
    priv->session = NULL;

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

static ERL_NIF_TERM espotify_player_current_track(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    espotify_private *priv = (espotify_private *)enif_priv_data(env);
    ASSERT_STARTED(priv);

    if (!spotifyctl_has_current_track())
        return enif_make_atom(env, "undefined");

    return OK_TERM(env, track_tuple(env, spotifyctl_get_session(), spotifyctl_current_track(), 1));
}

static ERL_NIF_TERM espotify_track_info(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    espotify_private *priv = (espotify_private *)enif_priv_data(env);
    ASSERT_STARTED(priv);

    char link[MAX_LINK];
    char *error_msg;

    if (enif_get_string(env, argv[0], link, MAX_LINK, ERL_NIF_LATIN1) < 1)
        return enif_make_badarg(env);

    ERL_NIF_TERM *reference = obtain_reference(priv->callback_env);
    ERL_NIF_TERM myref = enif_make_copy(env, *reference);

    if (spotifyctl_track_info(link, (void *)reference, &error_msg) == CMD_RESULT_ERROR) {
        return STR_ERROR(env, error_msg);
    }
    return enif_make_tuple2(env, enif_make_atom(env, "ok"), myref);
}


static ERL_NIF_TERM espotify_browse_album(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    espotify_private *priv = (espotify_private *)enif_priv_data(env);
    ASSERT_STARTED(priv);

    char link[MAX_LINK];
    char *error_msg;

    if (enif_get_string(env, argv[0], link, MAX_LINK, ERL_NIF_LATIN1) < 1)
        return enif_make_badarg(env);

    ERL_NIF_TERM *reference = obtain_reference(priv->callback_env);
    
    if (spotifyctl_browse_album(link, (void *)reference, &error_msg) == CMD_RESULT_ERROR) {
        return STR_ERROR(env, error_msg);
    }
    return enif_make_tuple2(env, enif_make_atom(env, "ok"), *reference);
}

static ERL_NIF_TERM espotify_browse_artist(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    espotify_private *priv = (espotify_private *)enif_priv_data(env);
    ASSERT_STARTED(priv);

    char link[MAX_LINK];
    char *error_msg;

    if (enif_get_string(env, argv[0], link, MAX_LINK, ERL_NIF_LATIN1) < 1)
        return enif_make_badarg(env);

    char atom[MAX_ATOM];

    if (!enif_get_atom(env, argv[1], atom, MAX_ATOM, ERL_NIF_LATIN1))
        return enif_make_badarg(env);
    
    sp_artistbrowse_type type = 0;
    if (!strcmp(atom, "full")) {
        type = SP_ARTISTBROWSE_FULL;
    } else if (!strcmp(atom, "no_tracks")) {
        type = SP_ARTISTBROWSE_NO_TRACKS;
    } else if (!strcmp(atom, "no_albums")) {
        type = SP_ARTISTBROWSE_NO_ALBUMS;
    } else {
        return enif_make_badarg(env);
    }

    ERL_NIF_TERM *reference = obtain_reference(priv->callback_env);
    
    if (spotifyctl_browse_artist(link, type, (void *)reference, &error_msg) == CMD_RESULT_ERROR) {
        return STR_ERROR(env, error_msg);
    }
    return enif_make_tuple2(env, enif_make_atom(env, "ok"), *reference);
}

static ERL_NIF_TERM espotify_load_image(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    espotify_private *priv = (espotify_private *)enif_priv_data(env);
    ASSERT_STARTED(priv);

    char link[MAX_LINK];
    char *error_msg;

    if (enif_get_string(env, argv[0], link, MAX_LINK, ERL_NIF_LATIN1) < 1)
        return enif_make_badarg(env);

    ERL_NIF_TERM *reference = (ERL_NIF_TERM *)enif_alloc(sizeof(ERL_NIF_TERM));
    *reference = enif_make_ref(priv->callback_env);

    if (spotifyctl_load_image(link, (void *)reference, &error_msg) == CMD_RESULT_ERROR) {
        return STR_ERROR(env, error_msg);
    }
    return enif_make_tuple2(env, enif_make_atom(env, "ok"), *reference);
}

static ERL_NIF_TERM espotify_search(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    espotify_private *priv = (espotify_private *)enif_priv_data(env);
    ASSERT_STARTED(priv);

    const ERL_NIF_TERM *ar;
    int c;

    if (!enif_get_tuple(env, argv[0], &c, &ar))
        return enif_make_badarg(env);

    if (c != 11)
        return enif_make_badarg(env);

    char atom[MAX_ATOM];
    spotifyctl_search_query query;

    if (!enif_get_atom(env, ar[0], atom, MAX_ATOM, ERL_NIF_LATIN1))
        return enif_make_badarg(env);
    if (strcmp(atom, "sp_search_query") != 0)
        return enif_make_badarg(env);
    if (enif_get_string(env, ar[1], query.query, MAX_LINK, ERL_NIF_LATIN1) < 1)
        return enif_make_badarg(env);
    if (!enif_get_uint(env, ar[2], &query.track_offset))
        return enif_make_badarg(env);
    if (!enif_get_uint(env, ar[3], &query.track_count))
        return enif_make_badarg(env);
    if (!enif_get_uint(env, ar[4], &query.album_offset))
        return enif_make_badarg(env);
    if (!enif_get_uint(env, ar[5], &query.album_count))
        return enif_make_badarg(env);
    if (!enif_get_uint(env, ar[6], &query.artist_offset))
        return enif_make_badarg(env);
    if (!enif_get_uint(env, ar[7], &query.artist_count))
        return enif_make_badarg(env);
    if (!enif_get_uint(env, ar[8], &query.playlist_offset))
        return enif_make_badarg(env);
    if (!enif_get_uint(env, ar[9], &query.playlist_count))
        return enif_make_badarg(env);
    if (!enif_get_atom(env, ar[10], atom, MAX_ATOM, ERL_NIF_LATIN1))
        return enif_make_badarg(env);
   
    if (!strcmp(atom, "standard")) {
        query.search_type = SP_SEARCH_STANDARD;
    } else if (!strcmp(atom, "suggest")) {
        query.search_type = SP_SEARCH_SUGGEST;
    } else
        return enif_make_badarg(env);

    ERL_NIF_TERM *reference = (ERL_NIF_TERM *)enif_alloc(sizeof(ERL_NIF_TERM));
    *reference = enif_make_ref(priv->callback_env);

    spotifyctl_search(query, (void *)reference);
    return enif_make_tuple2(env, enif_make_atom(env, "ok"), *reference);
}

static ERL_NIF_TERM espotify_load_playlistcontainer(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    espotify_private *priv = (espotify_private *)enif_priv_data(env);
    ASSERT_STARTED(priv);

    ERL_NIF_TERM *reference = (ERL_NIF_TERM *)enif_alloc(sizeof(ERL_NIF_TERM));
    *reference = enif_make_ref(priv->callback_env);

    char *error_msg;
    if (spotifyctl_load_playlistcontainer((void *)reference, &error_msg) == CMD_RESULT_ERROR) {
        return STR_ERROR(env, error_msg);
    }
    return enif_make_tuple2(env, enif_make_atom(env, "ok"), *reference);
}

static ERL_NIF_TERM espotify_load_user_playlistcontainer(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    espotify_private *priv = (espotify_private *)enif_priv_data(env);
    ASSERT_STARTED(priv);

    char name[MAX_LINK];
    char *error_msg;

    if (enif_get_string(env, argv[0], name, MAX_LINK, ERL_NIF_LATIN1) < 1)
        return enif_make_badarg(env);

    ERL_NIF_TERM *reference = obtain_reference(priv->callback_env);
    ERL_NIF_TERM myref = enif_make_copy(env, *reference);
    
    if (spotifyctl_load_user_playlistcontainer(name, (void *)reference, &error_msg) == CMD_RESULT_ERROR) {
        return STR_ERROR(env, error_msg);
    }
    return enif_make_tuple2(env, enif_make_atom(env, "ok"), myref);
}

static ERL_NIF_TERM espotify_load_playlist(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    espotify_private *priv = (espotify_private *)enif_priv_data(env);
    ASSERT_STARTED(priv);

    char link[MAX_LINK];
    char *error_msg;

    if (enif_get_string(env, argv[0], link, MAX_LINK, ERL_NIF_LATIN1) < 1)
        return enif_make_badarg(env);

    ERL_NIF_TERM *reference = obtain_reference(priv->callback_env);
    ERL_NIF_TERM myref = enif_make_copy(env, *reference);
    
    if (spotifyctl_load_playlist(link, (void *)reference, &error_msg) == CMD_RESULT_ERROR) {
        return STR_ERROR(env, error_msg);
    }
    return enif_make_tuple2(env, enif_make_atom(env, "ok"), myref);
}


static int load(ErlNifEnv* env, void** priv_data, ERL_NIF_TERM load_info)
{
    espotify_private* data = (espotify_private *)enif_alloc(sizeof(espotify_private));

    data->session = 0;
    data->callback_env = enif_alloc_env();
    *priv_data = data;
    return 0;
}

static void unload(ErlNifEnv* env, void* priv_data)
{
    espotify_private *priv = (espotify_private *)priv_data;
    if (priv->session) {
        spotifyctl_stop();

        // wait for spotify thread to exit
        void *resp;
        enif_thread_join(priv->session->tid, &resp);

        enif_free(priv->session);
        priv->session = NULL;
    }
    /* enif_free_env(priv->callback_env); */
    /* enif_free_env(temp_env()); */
    /* enif_free(priv); */
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
    {"player_unload", 0, espotify_player_unload},
    {"player_current_track", 0, espotify_player_current_track},

    {"track_info", 1, espotify_track_info},
    {"browse_album", 1, espotify_browse_album},
    {"browse_artist", 2, espotify_browse_artist},
    {"load_image", 1, espotify_load_image},

    {"search", 1, espotify_search},

    {"load_playlistcontainer", 0, espotify_load_playlistcontainer},
    {"load_user_playlistcontainer", 1, espotify_load_user_playlistcontainer},
    {"load_playlist", 1, espotify_load_playlist}
    
};

ERL_NIF_INIT(espotify_nif,nif_funcs,load,NULL,NULL,unload)
