#ifndef espotify_callbacks
#define espotify_callbacks

#include <libspotify/api.h>

void esp_error_feedback(void *erl_pid, const char *callback_name, const char *msg);
void esp_atom_feedback(void *erl_pid, const char *callback_name, const char *atom);
void esp_logged_in_feedback(void *erl_pid, sp_session *session, sp_user *user);
void esp_player_load_feedback(void *erl_pid, sp_session *session, sp_track *track);
void esp_player_track_info_feedback(void *erl_pid, sp_session *session, void *reference, sp_track *track);
void esp_player_browse_album_feedback(void *erl_pid, sp_session *session, void *reference, sp_albumbrowse *albumbrowse);
void esp_player_playlist_container_feedback(void *erl_pid, sp_session *session, void *refptr, sp_playlistcontainer *container);

#endif
