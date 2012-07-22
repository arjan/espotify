
-module(espotify_nif_playlist_SUITE).

-compile(export_all).

-include_lib("espotify.hrl").
-include_lib("espotify_tests_common.hrl").

all() ->
    [
%     test_load_playlistcontainer,
     test_load_playlist
    ].


test_load_playlistcontainer(_) ->
    ok = espotify_nif:set_pid(self()),

    %%{ok, Ref1} = espotify_nif:load_playlistcontainer(),
    %% Wait for my own playlist container
    {ok, {undefined, R}} = expect_callback(load_playlistcontainer),
    ct:print("~p", [R]),
    ok.


test_load_playlist(_) ->
    ok = espotify_nif:set_pid(self()),

    {ok, Ref1} = espotify_nif:load_playlist("spotify:user:acscherp:playlist:22iC2x9eVbU7HquIBeRECr"),
    %% Wait for my own playlist container
    {ok, {Ref1, R}} = expect_callback(load_playlist),
    ct:print("~p", [R]),
    ok.