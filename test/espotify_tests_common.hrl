%% Shared functions

init_per_suite(_Config) ->
    {ok, Username} = application:get_env(espotify, username),
    {ok, Password} = application:get_env(espotify, password),
    ok = espotify_nif:start(self(), "tmp", "tmp", Username, Password),
    expect_callback(logged_in),
    _Config.

end_per_suite(_Config) ->
    espotify_nif:stop(),
    _Config.

expect_callback(Callback) ->
    receive
        {'$spotify_callback', Callback, Result} ->
            Result;
        R ->
            ct:print("???? ~p", [R]),
            throw({error, bad_response})
    after
        20000 ->
            ct:print("No response"),
            throw({error, no_response})
    end.

