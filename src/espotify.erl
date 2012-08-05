%% @doc Erlang library for libspotify.
%%
%% Include <tt>espotify.hrl</tt> to get access to the records that are
%% used with this library.

-module(espotify).
-export([start/0]).

-include_lib("espotify.hrl").

-type sp_user() :: #sp_user{
     link :: string(),
     canonical_name :: binary(),
     display_name :: binary()
   }.


-type sp_artist() :: #sp_artist{
     is_loaded :: boolean(),
     link :: string(),
     name :: binary(),
     portrait :: string()
   }.

-type sp_album() :: #sp_album{
     is_loaded :: boolean(),
     link :: string(),
     is_available :: boolean(),
     artist :: #sp_artist{},
     cover :: string(),
     name :: binary(),
     year :: non_neg_integer(),
     type :: album | single | compilation | unknown
   }.

-type sp_track() :: #sp_track{
     is_loaded :: boolean(),
     link :: string(),
     is_local :: boolean(),
     is_starred :: boolean(),
     artists :: [#sp_artist{}],
     album :: #sp_album{},
     name :: binary(),
     duration :: non_neg_integer(),
     popularity :: non_neg_integer(),
     disc :: non_neg_integer(),
     index :: non_neg_integer()
   }.

-type sp_albumbrowse() :: #sp_albumbrowse{
     album :: #sp_album{},
     artist :: #sp_artist{},
     copyrights :: [binary()],
     tracks :: [#sp_track{}],
     review :: binary()
   }.

-type sp_artistbrowse() :: #sp_artistbrowse{
     artist :: #sp_artist{},
     portraits :: [string()],
     tracks :: [#sp_track{}],
     tophit_tracks :: [#sp_track{}],
     albums :: [#sp_album{}],
     similar_artists :: [#sp_artist{}],
     biography :: binary()
   }.

-type sp_image() :: #sp_image{
     format :: unknown | jpeg,
     data :: binary(),
     id :: binary()
   }.

-type sp_playlist_track() :: #sp_playlist_track{
     track :: #sp_track{},
     create_time  :: non_neg_integer(),
     creator :: #sp_user{},
     seen :: boolean(),
     message :: binary()
   }.

-type sp_playlist() :: #sp_playlist{
     is_loaded :: boolean(),
     link :: string(),
     name :: binary(),
     owner :: #sp_user{},
     collaborative :: boolean(),
     description :: binary(),
     image :: string(),
     num_tracks :: non_neg_integer(),
     tracks :: [#sp_playlist_track{}]
   }.

-type sp_playlistcontainer() :: #sp_playlistcontainer{
     owner :: #sp_user{},
     contents :: [#sp_playlist{} |
                  not_loaded |
                  {start_folder, Id::integer(), Name::binary()} | 
                  end_folder | 
                  placeholder]
   }.

-type sp_search_query() :: #sp_search_query{
     q :: binary(),
     track_offset :: integer,
     track_count :: integer,
     album_offset :: integer,
     album_count :: integer,
     artist_offset :: integer,
     artist_count :: integer,
     playlist_offset :: integer,
     playlist_count :: integer,
     type :: standard | suggest
   }.

-type sp_search_result() :: #sp_search_result{
     q :: binary(),
     did_you_mean :: binary(),
     total_tracks :: non_neg_integer(),
     total_albums :: non_neg_integer(),
     total_artists :: non_neg_integer(),
     total_playlists :: non_neg_integer(),
     tracks :: [#sp_track{}],
     albums :: [#sp_album{}],
     artists :: [#sp_artist{}],
     playlists :: [#sp_playlist{}]
   }.

-export_type([sp_user/0, sp_artist/0, sp_album/0, sp_track/0,
              sp_albumbrowse/0, sp_artistbrowse/0, sp_image/0,
              sp_playlist_track/0, sp_playlist/0,
              sp_playlistcontainer/0, sp_search_query/0,
              sp_search_result/0]).



%% API

start() ->
    ok = application:start(espotify).
