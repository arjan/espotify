#ifndef async_state_h
#define async_state_h

typedef struct _qitem_t
{
    struct _qitem_t*    next;
    ERL_NIF_TERM        term;
} qitem_t;

typedef struct
{
    ErlNifMutex*        lock;
    ErlNifCond*         cond;
    qitem_t*            head;
    qitem_t*            tail;
    int                 stopping;
} queue_t;

typedef struct
{
    ErlNifThreadOpts*   opts;
    ErlNifTid           qthread;
    queue_t*            queue;
    ErlNifPid           pid; /* the calling process for sending async feedback. */
    ErlNifEnv*          env;
    ErlNifEnv*          ref_env;
} async_state_t;

void async_set_pid(async_state_t *state, ErlNifPid pid);

async_state_t *async_start();
void async_stop(async_state_t *state);

ErlNifEnv *async_env_acquire(async_state_t *state);
void async_env_release_and_send(async_state_t *state, ERL_NIF_TERM term);

ERL_NIF_TERM *async_make_ref(async_state_t *state);
ERL_NIF_TERM async_return_ref(async_state_t *state, ERL_NIF_TERM *refptr);

#endif