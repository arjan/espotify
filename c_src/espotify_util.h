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
ERL_NIF_TERM user_tuple(ErlNifEnv* env, sp_session *sess, sp_user *user);
ERL_NIF_TERM albumbrowse_tuple(ErlNifEnv* env, sp_session *sess, sp_albumbrowse *albumbrowse);
ERL_NIF_TERM artistbrowse_tuple(ErlNifEnv* env, sp_session *sess, sp_artistbrowse *artistbrowse);
ERL_NIF_TERM image_tuple(ErlNifEnv* env, sp_session *sess, sp_image *image);
ERL_NIF_TERM search_result_tuple(ErlNifEnv* env, sp_session *sess, sp_search *search);
ERL_NIF_TERM playlistcontainer_tuple(ErlNifEnv* env, sp_session *sess, sp_playlistcontainer *container);
ERL_NIF_TERM playlist_tuple(ErlNifEnv* env, sp_session *sess, sp_playlist *playlist, int recurse);

ERL_NIF_TERM make_atom(ErlNifEnv* env, const char* atom);
ERL_NIF_TERM make_binary(ErlNifEnv *env, const char *string);

#endif
