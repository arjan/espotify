
-module(espotify_nif_SUITE).

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

test_start_bad_login(_C) ->
    ok = espotify_nif:start(self(), "nonexistinguser", "password"),
    receive
        {'$spotify_callback', logged_in, {error, _}} ->
            ok;
        _ ->
            throw({error, bad_response})
    after
        10000 ->
            throw({error, no_response})
    end,
    espotify_nif:stop(),
    ok.

test_start(C) ->
    Username = proplists:get_value(username, C),
    Password = proplists:get_value(password, C),

    ok = espotify_nif:start(self(), Username, Password),
    receive
        {'$spotify_callback', logged_in, {ok, #sp_user{canonical_name=Username}}} ->
            ct:print("Login OK as ~s", [Username]),
            ok;
        R ->
            ct:print("???, ~p", [R]),
            throw({error, bad_response})
    after
        10000 ->
            throw({error, no_response})
    end.


test_start_again(C) ->
    Username = proplists:get_value(username, C),
    Password = proplists:get_value(password, C),
    {error, already_started} = espotify_nif:start(self(), Username, Password).


test_stop(_) ->
    ok = espotify_nif:stop(),
    {error, not_started} = espotify_nif:stop().
