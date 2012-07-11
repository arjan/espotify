-module(espotify).
-export([start/0]).

%% API

start() ->
    ok = application:start(espotify).
