
-module(espotify_nif_browse_SUITE).

-compile(export_all).

-include_lib("espotify.hrl").


all() ->
    [
     test_track_info,
     test_browse_album,
     test_browse_artist,
     test_browse_artist_full,
     test_image
    ].

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


test_track_info(_) ->
    ok = espotify_nif:set_pid(self()),

    {error, "Parsing link failed"} = espotify_nif:track_info("fdsalfjdsaflldsafads"),

    %% Get info for a track
    {ok, Ref1} = espotify_nif:track_info("spotify:track:42H8K72L4HggbJgGAwqWgT"),
    %% Wait for the callback with the track info
    {ok, {Ref1, T=#sp_track{}}} = expect_callback(track_info),

    %% Test a track
    ct:print("A track:~n~p", [T]),
    "Carousel" = T#sp_track.name,

    Album = T#sp_track.album,
    "Next To Me" = Album#sp_album.name,
    2010 = Album#sp_album.year,

    [Artist] = T#sp_track.artists,
    "Ilse DeLange" = Artist#sp_artist.name,
    "spotify:artist:3FTKP1k9VbOng3m1rgnsqx" = Artist#sp_artist.link,

    %% load some more
    {ok, _} = espotify_nif:track_info("spotify:track:2feyopBofIiN35tyhGtlZD"),
    {ok, _} = espotify_nif:track_info("spotify:track:4vrcVOWlA2B1pMAMmohaeL"),
    {ok, _} = espotify_nif:track_info("spotify:track:5Xs1BaGAJ6tCb79VFfcyeu"),
    {ok, _} = espotify_nif:track_info("spotify:track:5YKzk2Kfd7ESyaAbQUKjpT"),
    {ok, _} = expect_callback(track_info),
    {ok, _} = expect_callback(track_info),
    {ok, _} = expect_callback(track_info),
    {ok, _} = expect_callback(track_info),

    ok.


test_browse_album(_) ->
    ok = espotify_nif:set_pid(self()),

    {error, "Parsing link failed"} = espotify_nif:browse_album("fdsalfjdsaflldsafads"),

    %% Get info for a track
    {ok, Ref1} = espotify_nif:browse_album("spotify:album:6WgGWYw6XXQyLTsWt7tXky"),
    {ok, {Ref1, AlbumBrowse=#sp_albumbrowse{}}} = expect_callback(browse_album),
    
    ct:print("~p", [AlbumBrowse]),

    "Graceland 25th Anniversary Edition" = AlbumBrowse#sp_albumbrowse.album#sp_album.name,
    "Paul Simon" = AlbumBrowse#sp_albumbrowse.artist#sp_artist.name,

    [FirstTrack|_] = AlbumBrowse#sp_albumbrowse.tracks,
    "The Boy In The Bubble" = FirstTrack#sp_track.name,

    ok.

test_browse_artist(_) ->
    ok = espotify_nif:set_pid(self()),

    {error, "Parsing link failed"} = espotify_nif:browse_artist("fdsalfjdsaflldsafads", no_tracks),

    {ok, Ref1} = espotify_nif:browse_artist("spotify:artist:2CvCyf1gEVhI0mX6aFXmVI", no_tracks),
    {ok, {Ref1, ArtistBrowse=#sp_artistbrowse{}}} = expect_callback(browse_artist),
    ct:print("~p", [ArtistBrowse]),

    "Paul Simon" = ArtistBrowse#sp_artistbrowse.artist#sp_artist.name,
    
    true = (length(ArtistBrowse#sp_artistbrowse.tracks) == 0),
    true = (length(ArtistBrowse#sp_artistbrowse.albums) > 0),
    ok.

test_browse_artist_full(_) ->
    ok = espotify_nif:set_pid(self()),

    {ok, Ref1} = espotify_nif:browse_artist("spotify:artist:2CvCyf1gEVhI0mX6aFXmVI", full),
    {ok, {Ref1, ArtistBrowse=#sp_artistbrowse{}}} = expect_callback(browse_artist),
    ct:print("~p", [ArtistBrowse]),

    "Paul Simon" = ArtistBrowse#sp_artistbrowse.artist#sp_artist.name,
    
    true = (length(ArtistBrowse#sp_artistbrowse.tracks) > 0),
    true = (length(ArtistBrowse#sp_artistbrowse.albums) > 0),
    ok.


test_image(_) ->
    ok = espotify_nif:set_pid(self()),
    {ok, Ref1} = espotify_nif:load_image("spotify:image:930de222ed6ad8bacf7eacbcb09214818a9e6b3b"),
    {ok, {Ref1, Image=#sp_image{}}} = expect_callback(load_image),

    %ct:print("~p", [Image]),
    ok = file:write_file("/tmp/test.jpg", Image#sp_image.data),
    ct:print("Written test image to /tmp/test.jpg; inspect for yourself :)"),
    ok.
    
