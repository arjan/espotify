#ifndef espotify_util
#define espotify_util

#include "erl_nif.h"
#include "spotifyctl/spotifyctl.h"

#define ERROR_TERM(env, term) enif_make_tuple2(env, enif_make_atom(env, "error"), term)
#define OK_TERM(env, term) enif_make_tuple2(env, enif_make_atom(env, "ok"), term)

ERL_NIF_TERM track_tuple(ErlNifEnv* env, spotifyctl_track *track);

#endif
