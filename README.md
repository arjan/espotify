espotify
========

Erlang wrapper for [libspotify](https://developer.spotify.com/technologies/libspotify/).

Current state
-------------

This library is experimental, and a work in progress. Its end goal is
to cover the entire API of libspotify. The coverage so far:

 * Playback of audio (uses native audio library; Alsa or Coreaudio)
 * Browse album
 * Browse artist
 * Get playlist information
 * Search tracks/artists/playlists
 * Get image data (for artist portraits, covers)

The things missing are mostly playlist and playlist container
management.


Installation and setup
----------------------

First of all, you need a Spotify Premium account to use this library.

[Download
libspotify](https://developer.spotify.com/technologies/libspotify/)
for your platform, and install it (on Linux / Mac, that would be
something like `make install prefix=/usr/local`; see libspotify's
README file for more info). Make sure the libspotify example programs
compile (especially the jukebox one to play sound, as espotify uses
the same libraries).

Next, [generate a libspotify app
key](https://developer.spotify.com/technologies/libspotify/keys/) and
download the resulting C-code, placing it in espotify's
`c_src/spotifyctl/appkey.c` file.

That should be it! Now, from espotify's directory, `rebar compile` should
compile the Erlang NIF library.


How to use
----------

The library is highly asynchronous by nature. After most commands you
need to wait for a response from spotify before you can issue the
next, as you can see in the following example:

    espotify_api:start(self(), "/tmp/espotify", "/tmp/espotify", "username", "password"),
    receive
        {'$spotify_callback', logged_in, {ok, _User}} ->
            ok
    end,
    case espotify_api:player_load("spotify:track:6JEK0CvvjDjjMUBFoXShNZ") of
        loading -> % wait for track to load
            receive
                {'$spotify_callback', player_load, loaded} ->
                    ok
            end;
        ok -> % already loaded
            ok
    end,
    espotify_api:player_play(true).

This logs you in to Spotify, waits for an 'ok' message from the
library; load a track, wait for it to be loaded, and finally plays
this track (in case you're wondering, it's "Never gonna give you up"
by Rick Astley :P)


Running the tests
-----------------

espotify comes with a bunch of CT test suites; to run it, create a
`test/app.config` with your spotify credentials (copy
`test/app.config.in` as example); and then run the tests:

    rebar ct -v
    
If everything is OK, you should see the tests logging in to Spotify,
playing a part of a song, etc.
