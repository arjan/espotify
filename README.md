espotify
========

Erlang wrapper for libspotify.

Installation and setup
----------------------

First of all, you need a Spotify Premium account to use this library.

Download libspotify from
https://developer.spotify.com/technologies/libspotify/ for your
platform, and install it (on Linux / Mac, that would be something like
`make install prefix=/usr/local` (see its README file for more info).

Next, generate an app key
(https://developer.spotify.com/technologies/libspotify/keys/) and
download the resulting C-code, placing it in espotify's
`c_src/spotifyctl/appkey.c` file.

That should be it; from espotify's director, `rebar compile` should
compile the Erlang NIF library.


Running the tests
-----------------

espotify comes with a CT test suites; to run it, create a
`test/app.config` with your spotify credentials (copy
`test/app.config.in` as example); and then run the tests:

    rebar compile ct -v
    
If everything is OK, you should see the tests logging in to Spotify,
playing a part of a song, etc.



