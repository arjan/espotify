
-module(espotify_nif_SUITE).

-compile(export_all).


all() ->
    [
     test_start,
     test_stop
    ].

init_per_suite(Config) ->
    {ok, Username} = application:get_env(espotify, username),
    {ok, Password} = application:get_env(espotify, password),
    [{username, Username}, {password, Password}
     | Config].

end_per_suite(R) ->
    R.

test_start(C) ->
    Username = proplists:get_value(username, C),
    Password = proplists:get_value(password, C),

    ok = espotify_nif:start(self(), Username, Password),
    {error, already_started} = espotify_nif:start(self(), Username, Password).


test_stop(_) ->
    ok = espotify_nif:stop(),
    {error, not_started} = espotify_nif:stop().
