
-module(espotify_nif_browse_SUITE).

-compile(export_all).

-include_lib("espotify.hrl").


all() ->
    [
     test_track_info
    ].

init_per_suite(Config) ->
    {ok, Username} = application:get_env(espotify, username),
    {ok, Password} = application:get_env(espotify, password),
    ok = espotify_nif:start(self(), "/tmp/espotify_nif", "/tmp/espotify_nif", Username, Password),
    expect_callback(logged_in),
    Config.

end_per_suite(R) ->
    espotify_nif:stop(),
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


test_track_info(_) ->
    ok = espotify_nif:set_pid(self()),

    {error, "Parsing track failed"} = espotify_nif:track_info("fdsalfjdsaflldsafads"),

    %% Get info for a track
    {ok, Ref1} = espotify_nif:track_info("spotify:track:42H8K72L4HggbJgGAwqWgT"),
    %% Wait for the callback with the track info
    {ok, {Ref1, #sp_track{}}} = expect_callback(track_info),

    %% some more
    {ok, _} = espotify_nif:track_info("spotify:track:2feyopBofIiN35tyhGtlZD"),
    {ok, _} = espotify_nif:track_info("spotify:track:4vrcVOWlA2B1pMAMmohaeL"),
    {ok, _} = expect_callback(track_info),
    {ok, _} = expect_callback(track_info),

    ok.
