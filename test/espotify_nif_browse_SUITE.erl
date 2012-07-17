
-module(espotify_nif_browse_SUITE).

-compile(export_all).

-include_lib("espotify.hrl").


all() ->
    [
    ].

init_per_suite(Config) ->
    {ok, Username} = application:get_env(espotify, username),
    {ok, Password} = application:get_env(espotify, password),
    ok = espotify_nif:start(self(), "/tmp/espotify_nif", "/tmp/espotify_nif", Username, Password),
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

