/* espotify.c */
#include <unistd.h> /* sleep */
#include <stdio.h> /* printf */

#include "spotifyctl/spotifyctl.h"
#include "erl_nif.h"


#define USERNAMEMAX 255

typedef struct  {
    char username[USERNAMEMAX];
    char password[USERNAMEMAX];
    ErlNifTid tid;
} espotify_session;

typedef struct
{
    ErlNifResourceType* resource_type;
    espotify_session *session;
} espotify_private;


void *run_main_thread(void *data)
{
    espotify_session *session = (espotify_session *)data;
    fprintf(stderr, "starting... %s\n\r", session->username);
    
    spotifyctl_run(session->username, session->password);
    fprintf(stderr, "done?\n\r");
    return NULL;
}

static ERL_NIF_TERM espotify_start(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{

    espotify_private *priv = enif_priv_data(env);

    if (priv->session) {
        // Allow only one simultaneous session!
        return enif_make_badarg(env);
    }

    priv->session = enif_alloc_resource(priv->resource_type, sizeof(espotify_session));

    if (enif_get_string(env, argv[0], priv->session->username, USERNAMEMAX, ERL_NIF_LATIN1) < 1)
        return enif_make_badarg(env);

    if (enif_get_string(env, argv[1], priv->session->password, USERNAMEMAX, ERL_NIF_LATIN1) < 1)
        return enif_make_badarg(env);
    
    if (enif_thread_create("espotify main loop", &priv->session->tid, run_main_thread, priv->session, NULL))
        return enif_make_badarg(env);

    return enif_make_atom(env, "ok");
}

static ERL_NIF_TERM espotify_stop(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{

    espotify_private *priv = enif_priv_data(env);

    if (!priv->session) {
        // No session started
        return enif_make_badarg(env);
    }

    void *resp;
    if (!spotifyctl_stop())
        return enif_make_badarg(env);

    // wait for spotify thread to exit
    enif_thread_join(priv->session->tid, &resp);

    // clean up
    enif_release_resource(priv->session);
    priv->session = 0;

    return enif_make_atom(env, "ok");
}

static int load(ErlNifEnv* env, void** priv_data, ERL_NIF_TERM load_info)
{
    espotify_private* data = enif_alloc(sizeof(espotify_private));

    data->resource_type =  enif_open_resource_type(env,NULL,"eSpotify",
                                                   NULL,
                                                   ERL_NIF_RT_CREATE, NULL);
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
    {"start", 2, espotify_start},
    {"stop", 0, espotify_stop}
};

ERL_NIF_INIT(espotify,nif_funcs,load,NULL,NULL,unload)
