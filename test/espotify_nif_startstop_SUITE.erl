
-module(espotify_nif_startstop_SUITE).

-compile(export_all).

-include_lib("espotify.hrl").


all() ->
    [
     test_start_bad_login,
     test_start,
     test_start_again,
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
    {ok, U=#sp_user{canonical_name=UsernameBin}} = expect_callback(logged_in),
    UsernameBin = list_to_binary(Username),
    ct:print("Login OK as ~s", [U#sp_user.link]),
    ok.


test_start_again(C) ->
    Username = proplists:get_value(username, C),
    Password = proplists:get_value(password, C),
    {error, already_started} = espotify_nif:start(self(), "/tmp/espotify_nif", "/tmp/espotify_nif", Username, Password).


test_stop(_) ->
    ok = espotify_nif:stop(),
    {error, not_started} = espotify_nif:stop().
