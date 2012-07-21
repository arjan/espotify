-record(
   sp_user,
   {
     link :: string(),
     canonical_name :: string(),
     display_name :: string()
   }).

-record(
   sp_artist,
   {
     is_loaded :: boolean(),
     link :: string(),
     name :: string(),
     portrait :: string()
   }).

-record(
   sp_album,
   {
     is_loaded :: boolean(),
     link :: string(),
     is_available = false :: boolean(),
     artist :: #sp_artist{},
     cover :: string(),
     name :: string(),
     year :: non_neg_integer(),
     type = undefined :: album | single | compilation | unknown
   }).

-record(
   sp_track,
   {
     is_loaded :: boolean(),
     link :: string(),
     is_local :: boolean(),
     is_starred :: boolean(),
     artists :: [#sp_artist{}],
     album :: #sp_album{},
     name :: string(),
     duration :: non_neg_integer(),
     popularity :: non_neg_integer(),
     disc :: non_neg_integer(),
     index :: non_neg_integer()
   }).

-record(
   sp_albumbrowse,
   {
     album :: #sp_album{},
     artist :: #sp_artist{},
     copyrights :: [string()],
     tracks :: [#sp_track{}],
     review :: string()
   }).

-record(
   sp_artistbrowse,
   {
     artist :: #sp_artist{},
     portraits :: [string()],
     tracks :: [#sp_track{}],
     tophit_tracks :: [#sp_track{}],
     albums :: [#sp_album{}],
     similar_artists :: [#sp_artist{}],
     biography :: string()
   }).

-record(
   sp_image,
   {
     format :: unknown | jpeg,
     data :: binary(),
     id :: binary()
   }).

-record(
   sp_playlist_track,
   {
   }).

-record(
   sp_playlist,
   {
     name :: string(),
     link :: string(),
     image :: string(),
     owner :: #sp_user{},
     collaborative :: boolean(),
     tracks :: [#sp_playlist_track{}]
   }).

-record(
   sp_playlistcontainer,
   {
     owner :: #sp_user{},
     contents :: [#sp_playlist{} | {start_folder, Id::integer(), Name::string()} | end_folder | placeholder]
   }).

-record(
   sp_search_query,
   {
     q :: string(),
     track_offset = 0 :: integer,
     track_count = 0 :: integer,
     album_offset = 0 :: integer,
     album_count = 0 :: integer,
     artist_offset = 0 :: integer,
     artist_count = 0 :: integer,
     playlist_offset = 0 :: integer,
     playlist_count = 0 :: integer,
     type = standard :: standard | suggest
   }).

-record(
   sp_search_result,
   {
     q :: string(),
     did_you_mean :: string(),
     total_tracks :: non_neg_integer(),
     total_albums :: non_neg_integer(),
     total_artists :: non_neg_integer(),
     total_playlists :: non_neg_integer(),
     tracks :: [#sp_track{}],
     albums :: [#sp_album{}],
     artists :: [#sp_artist{}],
     playlists :: [#sp_playlist{}]
   }).
