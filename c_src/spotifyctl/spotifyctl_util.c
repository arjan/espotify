#include <libspotify/api.h>
#include <stdlib.h>
#include <string.h>

#include "spotifyctl.h"


spotifyctl_track *make_track(sp_session *session, sp_track *track)
{
    spotifyctl_track *out = NULL;
    out = malloc(sizeof(spotifyctl_track));
    out->is_loaded = sp_track_is_loaded(track);

    sp_link *link = sp_link_create_from_track(track, 0);
    sp_link_as_string(link, out->link, MAX_LINK);
    sp_link_release(link);

    if (out->is_loaded)
    {
        out->is_starred = sp_track_is_starred(session, track);
        out->is_local = sp_track_is_local(session, track);

        out->track_name = malloc(strlen(sp_track_name(track))*sizeof(char));
        strcpy(out->track_name, sp_track_name(track));

        out->duration = sp_track_duration(track);
        out->popularity = sp_track_popularity(track);
        out->disc = sp_track_disc(track);
        out->index = sp_track_index(track);
    }
    return out;
}

void release_track(spotifyctl_track *track)
{
    if (track == NULL) return;
    if (track->is_loaded)
    {
        free(track->track_name);
    }
    free(track);
}
