/* espotify.c */
#include <unistd.h> /* sleep */
#include <stdio.h> /* printf */

#include "spotifyctl/spotifyctl.h"
#include "erl_nif.h"

#define ATOM_ERROR(env, s) (enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, s)))
#define USERNAMEMAX 255

typedef struct  {
    char username[USERNAMEMAX];
    char password[USERNAMEMAX];
    ErlNifPid pid; /* the calling process for sending async feedback. */
    ErlNifTid tid;
} espotify_session;

typedef struct
{
    espotify_session *session;
} espotify_private;


void *run_main_thread(void *data)
{
    espotify_session *session = (espotify_session *)data;
    spotifyctl_run(session->username, session->password);
    return NULL;
}

static ERL_NIF_TERM espotify_start(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    espotify_private *priv = enif_priv_data(env);

    if (argc != 3)
        return enif_make_badarg(env);
        
    if (priv->session) {
        // Allow only one simultaneous session!
        return ATOM_ERROR(env, "already_started");
    }

    priv->session = enif_alloc(sizeof(espotify_session));

    if (!enif_get_local_pid(env, argv[0], &priv->session->pid))
        return enif_make_badarg(env);

    if (enif_get_string(env, argv[1], priv->session->username, USERNAMEMAX, ERL_NIF_LATIN1) < 1)
        return enif_make_badarg(env);

    if (enif_get_string(env, argv[2], priv->session->password, USERNAMEMAX, ERL_NIF_LATIN1) < 1)
        return enif_make_badarg(env);

    if (enif_thread_create("espotify main loop", &priv->session->tid, run_main_thread, priv->session, NULL))
        return enif_make_badarg(env);
    fprintf(stderr, "starting..\n");

    return enif_make_atom(env, "ok");
}


static ERL_NIF_TERM espotify_stop(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    espotify_private *priv = enif_priv_data(env);
        
    if (!priv->session) {
        // No session started
        return ATOM_ERROR(env, "not_started");
    }

    void *resp;
    if (!spotifyctl_stop())
        return enif_make_badarg(env);

    // wait for spotify thread to exit
    enif_thread_join(priv->session->tid, &resp);

    // clean up
    enif_free(priv->session);
    priv->session = 0;

    return enif_make_atom(env, "ok");
}

static int load(ErlNifEnv* env, void** priv_data, ERL_NIF_TERM load_info)
{
    espotify_private* data = enif_alloc(sizeof(espotify_private));

    data->session = 0;
    
    *priv_data = data;
    return 0;
}

static void unload(ErlNifEnv* env, void* priv_data)
{
//    enif_free(priv_data);
}

static ErlNifFunc nif_funcs[] =
{
    {"start", 3, espotify_start},
    {"stop", 0, espotify_stop}
};

ERL_NIF_INIT(espotify_nif,nif_funcs,load,NULL,NULL,unload)
