
-module(espotify_nif_debug_SUITE).

-compile(export_all).

-include_lib("espotify.hrl").
-include_lib("espotify_tests_common.hrl").

all() ->
    [
     test_debug,
     test_debug,
     test_debug,
     test_debug,
     test_debug,
     test_debug,
     test_debug,
     test_debug,
     test_debug,
     test_debug,
     test_debug,
     test_debug,
     test_debug,
     test_debug,
     test_debug,
     test_debug
    ].



test_debug(_) ->
    ct:print("~p", [start]),
    ok = espotify_nif:set_pid(self()),
    [begin
         espotify_nif:debug()
         %expect_callback(debug)
     end || _ <- lists:seq(0,10000000)],
    ct:print("~p", [ok]).
