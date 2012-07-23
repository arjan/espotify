-record(
   sp_user,
   {
     link :: string(),
     canonical_name :: binary(),
     display_name :: binary()
   }).

-record(
   sp_artist,
   {
     is_loaded :: boolean(),
     link :: string(),
     name :: binary(),
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
     name :: binary(),
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
     name :: binary(),
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
     copyrights :: [binary()],
     tracks :: [#sp_track{}],
     review :: binary()
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
     biography :: binary()
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
     track :: #sp_track{},
     create_time  :: non_neg_integer(),
     creator :: #sp_user{},
     seen :: boolean(),
     message :: binary()
   }).

-record(
   sp_playlist,
   {
     is_loaded :: boolean(),
     link :: string(),
     name :: binary(),
     owner :: #sp_user{},
     collaborative :: boolean(),
     description :: binary(),
     image :: string(),
     num_tracks :: non_neg_integer(),
     tracks :: [#sp_playlist_track{}]
   }).

-record(
   sp_playlistcontainer,
   {
     owner :: #sp_user{},
     contents :: [#sp_playlist{} |
                  not_loaded |
                  {start_folder, Id::integer(), Name::binary()} | 
                  end_folder | 
                  placeholder]
   }).

-record(
   sp_search_query,
   {
     q :: binary(),
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
   }).
