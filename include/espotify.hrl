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
     name :: string(),
     portrait :: string()
   }).

-record(
   sp_album,
   {
     is_loaded :: boolean(),
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
     is_local :: boolean(),
     is_starred :: boolean(),
     link :: string(),
     %% artists :: [#sp_artist{}],
     %% album :: #sp_album{},
     track_name :: string()
     %% duration :: non_neg_integer(),
     %% popularity :: non_neg_integer(),
     %% disc :: non_neg_integer(),
     %% index :: non_neg_integer()
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
   sp_playlist,
   {
     name :: string(),
     uri :: string(),
     image_uri :: string()
   }).

-record(
   sp_search,
   {
     'query' :: string(),
     did_you_mean :: string(),
     total_tracks :: non_neg_integer(),
     total_albums :: non_neg_integer(),
     total_artists :: non_neg_integer(),
     total_playlists :: non_neg_integer(),
     tracks :: [#sp_track{}],
     albums :: [#sp_album{}],
     playlists :: [#sp_playlist{}],
     artists :: [#sp_artist{}]
   }).
