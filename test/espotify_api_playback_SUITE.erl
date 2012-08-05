
-module(espotify_api_playback_SUITE).

-compile(export_all).

-include_lib("espotify.hrl").
-include_lib("espotify_tests_common.hrl").

all() ->
    [
     test_player_no_current_track,
     test_player_load,
     test_player_play,
     test_player_seek,
     test_player_end_of_track,
     test_player_unload
    ].


test_player_no_current_track(_) ->
    ok = espotify_api:set_pid(self()),
    {error, no_current_track} = espotify_api:player_play(true),
    {error, no_current_track} = espotify_api:player_seek(112),
    {error, no_current_track} = espotify_api:player_unload().

test_player_load(_) ->
    ok = espotify_api:set_pid(self()),
    Link = "spotify:track:6JEK0CvvjDjjMUBFoXShNZ",
    CurrentTrack = case espotify_api:player_load(Link) of
                       loading ->
                           {ok, T} = expect_callback(player_load),
                           T;
                       {ok, T} -> 
                           T
                   end,
    Link = CurrentTrack#sp_track.link,
    ct:print("Current track: ~s (~s), ~p ms", [CurrentTrack#sp_track.link,
                                               CurrentTrack#sp_track.name,
                                               CurrentTrack#sp_track.duration]),
    ok.



%% @doc Some tests really rely on a human actually listening to the sound...
test_player_play(_) ->
    ok = espotify_api:set_pid(self()),
    ok = espotify_api:player_play(true),
    ct:print("Now playing!"),
    timer:sleep(3000),
    ok = espotify_api:player_play(false),
    timer:sleep(2000),
    ok.


test_player_seek(_) ->
    ok = espotify_api:set_pid(self()),
    ok = espotify_api:player_play(true),
    ok = espotify_api:player_seek(5000),
    timer:sleep(1000),
    ok = espotify_api:player_seek(5000),
    timer:sleep(1000),
    ok = espotify_api:player_play(false),
    ok.

%% @doc Seek to 500ms before end of track, wait for the EOT callback.
test_player_end_of_track(_) ->
    ok = espotify_api:set_pid(self()),
    {ok, T} = espotify_api:player_current_track(),
    Duration = T#sp_track.duration,
    ok = espotify_api:player_seek(Duration-500),
    ok = espotify_api:player_play(true),
    end_of_track = expect_callback(player_play),
    undefined = espotify_api:player_current_track(),
    ok.

test_player_unload(_) ->
    ok = espotify_api:set_pid(self()),
    Link = "spotify:track:6JEK0CvvjDjjMUBFoXShNZ",
    ok = espotify_api:player_load(Link), %% second load should be instantaneous; cached in libspotify.
    ok = espotify_api:player_unload(),
    {error, no_current_track} = espotify_api:player_unload().
