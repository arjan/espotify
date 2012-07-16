#include "spotifyctl/spotifyctl.h"

void esp_error_feedback(void *erl_pid, const char *callback_name, const char *msg);
void esp_player_load_feedback(void *erl_pid, spotifyctl_track *track);

void esp_logged_in_feedback(void *erl_pid, const char *link, const char *canonical_name, const char *display_name);

void esp_player_track_feedback(void *erl_pid, spotifyctl_track *track);
