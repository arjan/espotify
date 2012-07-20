#ifndef espotify_util
#define espotify_util

#include "erl_nif.h"
#include "spotifyctl/spotifyctl.h"

#include <libspotify/api.h>

#define ERROR_TERM(env, term) enif_make_tuple2(env, enif_make_atom(env, "error"), term)
#define OK_TERM(env, term) enif_make_tuple2(env, enif_make_atom(env, "ok"), term)
#define BOOL_TERM(env, cond) enif_make_atom(env, (cond) ? "true" : "false")

ERL_NIF_TERM artist_tuple(ErlNifEnv* env, sp_session *sess, sp_artist *artist);
ERL_NIF_TERM album_tuple(ErlNifEnv* env, sp_session *sess, sp_album *album, int recurse);
ERL_NIF_TERM track_tuple(ErlNifEnv* env, sp_session *session, sp_track *track, int recurse);

ErlNifEnv *temp_env();

#endif
