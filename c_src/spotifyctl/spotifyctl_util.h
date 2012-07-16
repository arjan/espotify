#ifndef spotifyctl_util_h
#define spotifyctl_util_h

spotifyctl_track *make_track(sp_session *session, sp_track *track);
void release_track(spotifyctl_track *track);


#endif
