
-module(espotify_nif_browse_SUITE).

-compile(export_all).

-include_lib("espotify.hrl").


all() ->
    [
     test_track_info,
     test_browse_album
    ].

init_per_suite(_Config) ->
    {ok, Username} = application:get_env(espotify, username),
    {ok, Password} = application:get_env(espotify, password),
    ok = espotify_nif:start(self(), "tmp", "tmp", Username, Password),
    expect_callback(logged_in),
    _Config.

end_per_suite(_Config) ->
    espotify_nif:stop(),
    _Config.

expect_callback(Callback) ->
    receive
        {'$spotify_callback', Callback, Result} ->
            Result;
        R ->
            ct:print("???? ~p", [R]),
            throw({error, bad_response})
    after
        20000 ->
            ct:print("No response"),
            throw({error, no_response})
    end.


test_track_info(_) ->
    ok = espotify_nif:set_pid(self()),

    {error, "Parsing link failed"} = espotify_nif:track_info("fdsalfjdsaflldsafads"),

    %% Get info for a track
    {ok, Ref1} = espotify_nif:track_info("spotify:track:42H8K72L4HggbJgGAwqWgT"),
    %% Wait for the callback with the track info
    {ok, {Ref1, T=#sp_track{}}} = expect_callback(track_info),

    %% Test a track
    ct:print("A track:~n~p", [T]),
    "Carousel" = T#sp_track.track_name,

    Album = T#sp_track.album,
    "Next To Me" = Album#sp_album.name,
    2010 = Album#sp_album.year,

    [Artist] = T#sp_track.artists,
    "Ilse DeLange" = Artist#sp_artist.name,
    "spotify:artist:3FTKP1k9VbOng3m1rgnsqx" = Artist#sp_artist.link,

    %% load some more
    {ok, _} = espotify_nif:track_info("spotify:track:2feyopBofIiN35tyhGtlZD"),
    {ok, _} = espotify_nif:track_info("spotify:track:4vrcVOWlA2B1pMAMmohaeL"),
    {ok, _} = espotify_nif:track_info("spotify:track:5Xs1BaGAJ6tCb79VFfcyeu"),
    {ok, _} = espotify_nif:track_info("spotify:track:5YKzk2Kfd7ESyaAbQUKjpT"),
    {ok, _} = expect_callback(track_info),
    {ok, _} = expect_callback(track_info),
    {ok, _} = expect_callback(track_info),
    {ok, _} = expect_callback(track_info),

    ok.


test_browse_album(_) ->
    ok = espotify_nif:set_pid(self()),

    %%{error, "Parsing link failed"} = espotify_nif:browse_album("fdsalfjdsaflldsafads"),
    %% Get info for a track
    {ok, Ref1} = espotify_nif:browse_album("spotify:album:6WgGWYw6XXQyLTsWt7tXky"),
    {ok, {Ref1, Album=#sp_albumbrowse{}}} = expect_callback(browse_album),
    ct:print("~p", [Album]),
    ok.
