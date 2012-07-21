
-module(espotify_nif_search_SUITE).

-compile(export_all).

-include_lib("espotify.hrl").
-include_lib("espotify_tests_common.hrl").

all() ->
    [
     %% test_search_track,
     %% test_search_album,
     test_search_artist
    ].


test_search_track(_) ->
    ok = espotify_nif:set_pid(self()),

    {'EXIT', {badarg, _}} = (catch espotify_nif:search(foo)),

    {ok, Ref1} = espotify_nif:search(#sp_search_query{q="Paul Simon", track_count=2}),
    {ok, {Ref1, R}} = expect_callback(search),

    2 = length(R#sp_search_result.tracks),
    #sp_track{} = hd(R#sp_search_result.tracks),
    
    true = (R#sp_search_result.total_tracks > length(R#sp_search_result.tracks)),
    ct:print("~p", [R]),
    ok.

test_search_album(_) ->
    ok = espotify_nif:set_pid(self()),

    {'EXIT', {badarg, _}} = (catch espotify_nif:search(foo)),

    {ok, Ref1} = espotify_nif:search(#sp_search_query{q="Paul Simon", album_count=2}),
    {ok, {Ref1, R}} = expect_callback(search),

    2 = length(R#sp_search_result.albums),
    #sp_album{} = hd(R#sp_search_result.albums),

    true = (R#sp_search_result.total_albums > length(R#sp_search_result.albums)),
    ct:print("~p", [R]),
    ok.


test_search_artist(_) ->
    ok = espotify_nif:set_pid(self()),

    {'EXIT', {badarg, _}} = (catch espotify_nif:search(foo)),

    {ok, Ref1} = espotify_nif:search(#sp_search_query{q="Simon", artist_count=2}),
    {ok, {Ref1, R}} = expect_callback(search),
    ct:print("~p", [R]),
    
    2 = length(R#sp_search_result.artists),
    #sp_artist{} = hd(R#sp_search_result.artists),

    true = (R#sp_search_result.total_artists > length(R#sp_search_result.artists)),
    ok.
