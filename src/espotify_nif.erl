-module(espotify_nif).

-export([
         start/3,
         stop/0,
         set_pid/1,

         player_load/1,
         player_prefetch/1,
         player_play/1,
         player_seek/1,
         player_unload/0
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

%% @doc Start the NIF spotify main loop. Only one spotify session per
%% VM is supported; this is a limitation of libspotify. The pid given
%% will receive callback messages.
-spec start(string(), string(), pid()) -> ok | {error, already_started}.
start(_, _, _) ->
    ?NOT_LOADED.

%% @doc Stop the main loop. No espotify commands can be called after
%% this.
-spec stop() -> ok | {error, not_started}.
stop() ->
    ?NOT_LOADED.
    
set_pid(_) ->
    ?NOT_LOADED.

player_load(_Track) ->
    ?NOT_LOADED.

player_prefetch(_Track) ->
    ?NOT_LOADED.

player_play(_Play) ->
    ?NOT_LOADED.

player_seek(_Offset) ->
    ?NOT_LOADED.

player_unload() ->
    ?NOT_LOADED.
