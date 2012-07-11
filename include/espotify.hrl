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
     name :: binary(),
     portrait :: binary()
   }).

-record(
   sp_album,
   {
     is_available = false :: boolean(),
     artist :: #sp_artist{},
     cover :: binary(),
     name :: binary(),
     year :: non_neg_integer(),
     type = undefined :: album | single | compilation | unknown
   }).

-record(
   sp_track,
   {
     is_local :: boolean(),
     is_starred :: boolean(),
     artists :: [#sp_artist{}],
     album :: #sp_album{},
     track_name :: binary(),
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
     portraits :: [binary()],
     tracks :: [#sp_track{}],
     tophit_tracks :: [#sp_track{}],
     albums :: [#sp_album{}],
     similar_artists :: [#sp_artist{}],
     biography :: binary()
   }).

-record(
   sp_playlist,
   {
     name :: binary(),
     uri :: binary(),
     image_uri :: binary()
   }).

-record(
   sp_search,
   {
     'query' :: binary(),
     did_you_mean :: binary(),
     total_tracks :: non_neg_integer(),
     total_albums :: non_neg_integer(),
     total_artists :: non_neg_integer(),
     total_playlists :: non_neg_integer(),
     tracks :: [#sp_track{}],
     albums :: [#sp_album{}],
     playlists :: [#sp_playlist{}],
     artists :: [#sp_artist{}]
   }).
