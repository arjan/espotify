
-module(espotify_api_debug_SUITE).

-compile(export_all).

-include_lib("espotify.hrl").
-include_lib("espotify_tests_common.hrl").

all() ->
    [
     test_debug,
     test_debug,
     test_debug
    ].



test_debug(_) ->
    ok = espotify_api:set_pid(self()),
    [begin
         espotify_api:debug(),
         expect_callback(debug)
     end || _ <- lists:seq(0,100000)],
    ct:print("~p", [ok]).
