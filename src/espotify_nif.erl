%% @doc The libspotify Erlang Interface. 
%%
%% <h3>Callback messages</h3> Many calls will at some point in time an
%% asynchronous callback message. For this purpose, the start/5
%% function has a pid() argument, and a set_pid/1 function is provided
%% to later change the callback pid.
%%
%% Callback messages are tagged tuples of the form
%% <pre>{'$spotify_callback', CallbackName::atom(), term()}</pre>,
%% where CallbackName is an atom named after the C function that
%% triggered the message; and term is a result term.
%%
%% @author Arjan Scherpenisse
-module(espotify_nif).

-include_lib("espotify.hrl").

-export([
         start/5,
         stop/0,
         set_pid/1,

         player_load/1,
         player_prefetch/1,
         player_play/1,
         player_seek/1,
         player_unload/0,
         player_current_track/0,

         track_info/1,
         browse_album/1,
         browse_artist/2,
         load_image/1,

         search/1,
         
         load_playlistcontainer/0,
         load_playlist/1
        ]).

-define(NOT_LOADED, throw({error, "NIF library not loaded"})).
-on_load(init/0).

init() ->
    SoName = case code:priv_dir(?MODULE) of
                 {error, bad_name} ->
                     case filelib:is_dir(filename:join(["..", "priv"])) of
                         true ->
                             filename:join(["..", "priv", "espotify_nif"]);
                         false ->
                             case filelib:is_dir(filename:join(["..", "..", "priv"])) of
                                 true ->
                                     filename:join(["..", "..", "priv", "espotify_nif"]);
                                 false ->
                                     filename:join(["priv", "espotify_nif"])
                             end
                     end;
                 Dir ->
                     filename:join(Dir, "espotify_nif")
             end,
    ok = erlang:load_nif(SoName, 0).

%% @doc Login to Spotify and start the main event loop. Only one
%% Spotify session per VM is supported; this is a limitation of
%% libspotify. The pid given will receive callback messages.
%%
%% On successful login, a message <pre>{'$spotify_callback', logged_in, {ok, #sp_user{}}} </pre> will be sent back.
-spec start(CallbackPid::pid(),
            CacheLocation::string(), SettingsLocation::string(),
            Username::string(), Password::string()) -> ok | {error, already_started}.
start(_, _, _, _, _) ->
    ?NOT_LOADED.

%% @doc Stop the main loop. No espotify commands can be called after
%% this.
-spec stop() -> ok | {error, not_started}.
stop() ->
    ?NOT_LOADED.
    
%% @doc Set the callback process id.
set_pid(_) ->
    ?NOT_LOADED.

%% @doc Load a track into the player for playback. `ok' will be
%% returned when the track was loaded immediately into the
%% player. When the track did not came from the cache, the atom
%% `loading' is returned, in this case you need to wait for the
%% message <pre>{'$spotify_callback', player_load, loaded}</pre>
%% before attempting to play the track.
-spec player_load(string()) -> loading | ok.
player_load(_Track) ->
    ?NOT_LOADED.

player_prefetch(_Track) ->
    ?NOT_LOADED.

%% @doc Toggle the play state of the currently loaded track.
-spec player_play(boolean()) -> ok | {error, term()}.
player_play(_Play) ->
    ?NOT_LOADED.

%% @doc Seek to position in the currently loaded track. The track offset is given in milliseconds.
-spec player_seek(Offset::non_neg_integer()) -> ok | {error, term()}.
player_seek(_Offset) ->
    ?NOT_LOADED.

%% @doc Stops the currently playing track.
-spec player_unload() -> ok | {error, term()}.
player_unload() ->
    ?NOT_LOADED.

%% @doc Returns the currently playing track.
-spec player_current_track() -> #sp_track{} | undefined.
player_current_track() ->
    ?NOT_LOADED.

%% @doc Get information about a single track. This call is
%% asynchronous and returns a reference object. The async callback
%% will include the same reference value so it can be mapped back.
%% 
%% Format of the callback structure:
%% <pre>{'$spotify_callback', track_info, {ok, {reference(), #sp_track{}}}} </pre>
-spec track_info(string()) -> {ok, reference()}.
track_info(_) ->
    ?NOT_LOADED.


%% @doc Browse information about an album, asynchronously.
%%
%% <pre>{'$spotify_callback', browse_album, {ok, {reference(), #sp_albumbrowse{}}}} </pre>
-spec browse_album(string()) -> {ok, reference()}.
browse_album(_) ->
    ?NOT_LOADED.

%% @doc Browse information about an artist, asynchronously.
%%
%% <pre>{'$spotify_callback', browse_artist, {ok, {reference(), #sp_artistbrowse{}}}} </pre>
-spec browse_artist(string(), full | no_tracks | no_albums) -> {ok, reference()}.
browse_artist(_, _) ->
    ?NOT_LOADED.

%% @doc Load image from link, asynchronously
%%
%% <pre>{'$spotify_callback', load_image, {ok, {reference(), #sp_image{}}}} </pre>
-spec load_image(string()) -> {ok, reference()}.
load_image(_) ->
    ?NOT_LOADED.


%% @doc Do a search request
%%
%% <pre>{'$spotify_callback', search, {ok, {reference(), #sp_search_result{}}}} </pre>
-spec search(#sp_search_result{}) -> {ok, reference()}.
search(_) ->
    ?NOT_LOADED.

%% @doc Load the playlist container of the current user.
%%
%% <pre>{'$spotify_callback', load_playlistcontainer, {ok, {reference(), #sp_playlistcontainer{}}}} </pre>
-spec load_playlistcontainer() -> {ok, reference()}.
load_playlistcontainer() ->
    ?NOT_LOADED.


%% @doc Load given playlist.
%%
%% <pre>{'$spotify_callback', load_playlist, {ok, {reference(), #sp_playlist{}}}} </pre>
-spec load_playlist(string()) -> {ok, reference()}.
load_playlist(_) ->
    ?NOT_LOADED.



