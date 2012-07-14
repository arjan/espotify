
-module(espotify_nif_SUITE).

-compile(export_all).

-include_lib("espotify.hrl").


all() ->
    [
     test_start_bad_login,
     test_start,
     test_start_again,
     test_player_load,
     test_player_play,
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
        10000 ->
            ct:print("No response"),
            throw({error, no_response})
    end.

test_start_bad_login(_C) ->
    ok = espotify_nif:start(self(), "nonexistinguser", "password"),
    {error, _} = expect_callback(logged_in),
    espotify_nif:stop(),
    ok.

test_start(C) ->
    Username = proplists:get_value(username, C),
    Password = proplists:get_value(password, C),

    ok = espotify_nif:start(self(), Username, Password),
    {ok, U=#sp_user{canonical_name=Username}} = expect_callback(logged_in),
    ct:print("Login OK as ~s", [U#sp_user.link]),
    ok.


test_start_again(C) ->
    Username = proplists:get_value(username, C),
    Password = proplists:get_value(password, C),
    {error, already_started} = espotify_nif:start(self(), Username, Password).


test_player_load(_) ->
    ok = espotify_nif:set_pid(self()),
    %spotify:track:6loflMnGf0yXWlIzqyHSJX
    %spotify:track:6JEK0CvvjDjjMUBFoXShNZ
    loading = espotify_nif:player_load("spotify:track:6loflMnGf0yXWlIzqyHSJX"),
    loaded = expect_callback(player_load),
    ok.
    

test_player_play(_) ->
    ok = espotify_nif:set_pid(self()),
    ok = espotify_nif:player_play(true),
    ct:print("Now playing!"),
    timer:sleep(5000),
    ok = espotify_nif:player_play(false),
    timer:sleep(2000),
    ok = espotify_nif:player_play(true),
    timer:sleep(4000),
    ok = espotify_nif:player_play(false),
    timer:sleep(1000),
    ct:print("done"),
    ok.
    
test_stop(_) ->
    ok = espotify_nif:stop(),
    {error, not_started} = espotify_nif:stop().
