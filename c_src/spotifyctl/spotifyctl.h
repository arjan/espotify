int spotifyctl_run(void *erl_pid, char *username, char *password);
void spotifyctl_set_pid(void *erl_pid);
int spotifyctl_has_current_track();

#define CMD_STOP 1
#define CMD_PLAYER_LOAD     10
#define CMD_PLAYER_PLAY     11
#define CMD_PLAYER_PREFETCH 12
#define CMD_PLAYER_SEEK     13
#define CMD_PLAYER_UNLOAD   14

#define CMD_RESULT_OK 0
#define CMD_RESULT_ERROR 1
#define CMD_RESULT_LOADING 2

int spotifyctl_do_cmd0(char cmd, char **error_msg);
int spotifyctl_do_cmd1(char cmd, void *arg1, char **error_msg);
//int spotifyctl_do_cmd2(spotifyctl_cmd cmd, void *arg1, void *arg2, char **error_msg);
