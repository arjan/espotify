-module(espotify).

-export([start/2]).

-on_load(init/0).

init() ->
    SoName = case code:priv_dir(?MODULE) of
                 {error, bad_name} ->
                     case filelib:is_dir(filename:join(["..", "priv"])) of
                         true ->
                             filename:join(["..", "priv", "espotify_nif"]);
                         false ->
                             filename:join(["priv", "espotify_nif"])
                     end;
                 Dir ->
                     filename:join(Dir, "espotify_nif")
             end,
    ok = erlang:load_nif(SoName, 0).


start(_Username, _Password) ->
    throw({error, "NIF library not loaded"}).

