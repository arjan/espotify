#ifndef espotify_callbacks
#define espotify_callbacks

#include <libspotify/api.h>

void esp_debug(void *erl_pid, void *reference);

void esp_error_feedback(void *erl_pid, const char *callback_name, const char *msg);
void esp_atom_feedback(void *erl_pid, const char *callback_name, const char *atom);
void esp_logged_in_feedback(void *erl_pid, sp_session *session, sp_user *user);
void esp_player_load_feedback(void *erl_pid, sp_session *session, sp_track *track);
void esp_player_track_info_feedback(void *erl_pid, sp_session *session, void *reference, sp_track *track);
void esp_player_browse_album_feedback(void *erl_pid, sp_session *session, void *reference, sp_albumbrowse *albumbrowse);
void esp_player_browse_artist_feedback(void *erl_pid, sp_session *session, void *reference, sp_artistbrowse *artistbrowse);
void esp_player_load_image_feedback(void *erl_pid, sp_session *session, void *reference, sp_image *image);

void esp_player_search_feedback(void *erl_pid, sp_session *session, void *reference, sp_search *search);

void esp_player_load_playlistcontainer_feedback(void *erl_pid, sp_session *session, void *refptr, sp_playlistcontainer *container);
void esp_player_load_playlist_feedback(void *erl_pid, sp_session *session, void *refptr, sp_playlist *playlist);

#endif
