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


void *start_session(void *data)
{
    espotify_session *session = (espotify_session *)data;
    spotifyctl_run(session->username, session->password);
    return NULL;
}

static ERL_NIF_TERM start(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
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
    
    if (enif_thread_create("espotify main loop", &priv->session->tid, start_session, priv->session, NULL))
        return enif_make_badarg(env);

    return enif_make_string(env, priv->session->username, ERL_NIF_LATIN1);
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
    {"start", 2, start}
};

ERL_NIF_INIT(espotify,nif_funcs,load,NULL,NULL,unload)
