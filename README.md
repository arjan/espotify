espotify
========

Erlang wrapper for [libspotify](https://developer.spotify.com/technologies/libspotify/).

Current state
-------------

This library is experimental, and a work in progress. Its end goal is
to cover the entire API of libspotify, but currently it is far from
complete. Use at your own risk! :-)


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

    espotify_nif:start(self(), "username", "password"),
    receive
        {'$spotify_callback', logged_in, {ok, _User}} ->
            ok
    end,
    case espotify_nif:player_load("spotify:track:6JEK0CvvjDjjMUBFoXShNZ") of
        loading -> % wait for track to load
            receive
                {'$spotify_callback', player_load, loaded} ->
                    ok
            end;
        ok -> % already loaded
            ok
    end,
    espotify_nif:player_play(true).

This logs you in to Spotify, waits for an 'ok' message from the
library; load a track, wait for it to be loaded, and finally plays
this track (in case you're wondering, it's "Never gonna give you up"
by Rick Astley :P)


Running the tests
-----------------

espotify comes with a CT test suites; to run it, create a
`test/app.config` with your spotify credentials (copy
`test/app.config.in` as example); and then run the tests:

    rebar compile ct -v
    
If everything is OK, you should see the tests logging in to Spotify,
playing a part of a song, etc.
