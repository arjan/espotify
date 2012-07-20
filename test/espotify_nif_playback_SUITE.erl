
-module(espotify_nif_playback_SUITE).

-compile(export_all).

-include_lib("espotify.hrl").


all() ->
    [
     test_start_bad_login,
     test_start,
     test_start_again,
     test_player_no_current_track,
     test_player_load,
     test_player_play,
     test_player_seek,
     test_player_end_of_track,
     test_player_unload,
     test_stop
    ].

init_per_suite(Config) ->
    {ok, Username} = application:get_env(espotify, username),
    {ok, Password} = application:get_env(espotify, password),
    [{username, Username}, {password, Password}
     | Config].

end_per_suite(R) ->
    R.

expect_callback(Callback) ->
    receive
        {'$spotify_callback', Callback, Result} ->
            Result;
        debug ->
            ct:print("Debug"),
            expect_callback(Callback);
        R ->
            ct:print("???? ~p", [R]),
            throw({error, bad_response})
    after
        20000 ->
            ct:print("No response"),
            throw({error, no_response})
    end.

test_start_bad_login(_C) ->
    ok = espotify_nif:start(self(), "/tmp/espotify_nif", "/tmp/espotify_nif", "nonexistinguser", "password"),
    {error, _} = expect_callback(logged_in),
    espotify_nif:stop(),
    ok.

test_start(C) ->
    Username = proplists:get_value(username, C),
    Password = proplists:get_value(password, C),

    ok = espotify_nif:start(self(), "/tmp/espotify_nif", "/tmp/espotify_nif", Username, Password),
    {ok, U=#sp_user{canonical_name=Username}} = expect_callback(logged_in),
    ct:print("Login OK as ~s", [U#sp_user.link]),
    ok.


test_start_again(C) ->
    Username = proplists:get_value(username, C),
    Password = proplists:get_value(password, C),
    {error, already_started} = espotify_nif:start(self(), "/tmp/espotify_nif", "/tmp/espotify_nif", Username, Password).


test_player_no_current_track(_) ->
    ok = espotify_nif:set_pid(self()),
    {error, no_current_track} = espotify_nif:player_play(true),
    {error, no_current_track} = espotify_nif:player_seek(112),
    {error, no_current_track} = espotify_nif:player_unload().


test_player_load(_) ->
    ok = espotify_nif:set_pid(self()),
    Link = "spotify:track:6JEK0CvvjDjjMUBFoXShNZ",
    CurrentTrack = case espotify_nif:player_load(Link) of
                       loading ->
                           {ok, T} = expect_callback(player_load),
                           T;
                       {ok, T} -> 
                           T
                   end,
    Link = CurrentTrack#sp_track.link,
    ct:print("Current track: ~s (~s), ~p ms", [CurrentTrack#sp_track.link,
                                               CurrentTrack#sp_track.track_name,
                                               CurrentTrack#sp_track.duration]),
    ok.



%% @doc Some tests really rely on a human actually listening to the sound...
test_player_play(_) ->
    ok = espotify_nif:set_pid(self()),
    ok = espotify_nif:player_play(true),
    ct:print("Now playing!"),
    timer:sleep(3000),
    ok = espotify_nif:player_play(false),
    timer:sleep(2000),
    ok.


test_player_seek(_) ->
    ok = espotify_nif:set_pid(self()),
    ok = espotify_nif:player_play(true),
    ok = espotify_nif:player_seek(5000),
    timer:sleep(1000),
    ok = espotify_nif:player_seek(5000),
    timer:sleep(1000),
    ok = espotify_nif:player_play(false),
    ok.

%% @doc Seek to 500ms before end of track, wait for the EOT callback.
test_player_end_of_track(_) ->
    ok = espotify_nif:set_pid(self()),
    {ok, T} = espotify_nif:player_current_track(),
    Duration = T#sp_track.duration,
    ok = espotify_nif:player_seek(Duration-500),
    ok = espotify_nif:player_play(true),
    end_of_track = expect_callback(player_play),
    undefined = espotify_nif:player_current_track(),
    ok.

test_player_unload(_) ->
    ok = espotify_nif:set_pid(self()),
    Link = "spotify:track:6JEK0CvvjDjjMUBFoXShNZ",
    ok = espotify_nif:player_load(Link), %% second load should be instantaneous; cached in libspotify.
    ok = espotify_nif:player_unload(),
    {error, no_current_track} = espotify_nif:player_unload().


test_stop(_) ->
    ok = espotify_nif:stop(),
    {error, not_started} = espotify_nif:stop().
