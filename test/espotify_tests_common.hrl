%% Shared functions

init_per_suite(_Config) ->
    {ok, Username} = application:get_env(espotify, username),
    {ok, Password} = application:get_env(espotify, password),
    {ok, TmpDir} = application:get_env(espotify, tmp_dir),
    ok = espotify_api:start(self(), TmpDir, TmpDir, Username, Password),
    expect_callback(logged_in),
    _Config.

end_per_suite(_Config) ->
    flush_messages(),
    espotify_api:stop(),
    _Config.

flush_messages() ->
    receive
        _X ->
            ct:print("... flushed: ~p", [_X]),
            flush_messages()
    after
        0 ->
            true
    end.
            
expect_callback(Callback) ->
    receive
        {'$spotify_callback', Callback, Result} ->
            %ct:print("~p: ~p", [Callback, Result]),
            Result;
        {'$spotify_callback', Cb2, _} ->
            ct:print("ignored: ~p", [Cb2]),
            expect_callback(Callback);
        R ->
            ct:print("???? ~p", [R]),
            throw({error, bad_response})
    after
        20000 ->
            ct:print("No response"),
            throw({error, no_response})
    end.

