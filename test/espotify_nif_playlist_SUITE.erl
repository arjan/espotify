
-module(espotify_nif_playlist_SUITE).

-compile(export_all).

-include_lib("espotify.hrl").
-include_lib("espotify_tests_common.hrl").

all() ->
    [
%     test_load_playlistcontainer,
%     test_load_user_playlistcontainer,
     test_load_playlist
    ].


test_load_playlistcontainer(_) ->
    ok = espotify_nif:set_pid(self()),

    %% Wait for my own playlist container
    {ok, {undefined, R=#sp_playlistcontainer{}}} = expect_callback(load_playlistcontainer),
    ct:print("~p", [R]),
    ok.


test_load_user_playlistcontainer(_) ->
    ok = espotify_nif:set_pid(self()),
    {ok, Ref1} = espotify_nif:load_user_playlistcontainer("Arjan Scherpenisse"),
    %% Wait for user's playlist container
    {ok, {Ref1, R}} = expect_callback(load_playlistcontainer),
    ct:print("~p", [R]),
    ok.


test_load_playlist(_) ->
    ok = espotify_nif:set_pid(self()),

    %% Load it; waits for all tracks to load.
    {ok, Ref1} = espotify_nif:load_playlist("spotify:user:acscherp:playlist:22iC2x9eVbU7HquIBeRECr"),
    {ok, {Ref1, R}} = expect_callback(load_playlist),
    ct:print("~p", [R]),

    %% Load it again; should be fast now its in RAM
    {ok, Ref2} = espotify_nif:load_playlist("spotify:user:acscherp:playlist:22iC2x9eVbU7HquIBeRECr"),
    {ok, {Ref2, R}} = expect_callback(load_playlist),
    ok.
