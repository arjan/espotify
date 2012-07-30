#include <assert.h>
#include <stdio.h>
#include "erl_nif.h"
#include "espotify_async.h"

#define DBG(x) fprintf(stderr, "DEBUG: " x "\n");


queue_t*
queue_create()
{
    queue_t* ret;

    ret = (queue_t*) enif_alloc(sizeof(queue_t));
    if(ret == NULL) return NULL;

    ret->lock = NULL;
    ret->cond = NULL;
    ret->head = NULL;
    ret->tail = NULL;

    ret->lock = enif_mutex_create("queue_lock");
    if(ret->lock == NULL) goto error;

    ret->cond = enif_cond_create("queue_cond");
    if(ret->cond == NULL) goto error;

    return ret;

error:
    if(ret->lock != NULL) enif_mutex_destroy(ret->lock);
    if(ret->cond != NULL) enif_cond_destroy(ret->cond);
    if(ret != NULL) enif_free(ret);
    return NULL;
}

void
queue_destroy(queue_t* queue)
{
    ErlNifMutex* lock;
    ErlNifCond* cond;

    enif_mutex_lock(queue->lock);
    //assert(queue->head == NULL && "Destroying a non-empty queue.");
    //assert(queue->tail == NULL && "Destroying queue in invalid state.");

    lock = queue->lock;
    cond = queue->cond;

    queue->lock = NULL;
    queue->cond = NULL;

    enif_mutex_unlock(lock);

    enif_cond_destroy(cond);
    enif_mutex_destroy(lock);
    enif_free(queue);
}

int
queue_push(queue_t* queue, ErlNifEnv *env, ERL_NIF_TERM term)
{
    qitem_t* item = (qitem_t*) enif_alloc(sizeof(qitem_t));
    if(item == NULL) return 0;

    item->term = term;
    item->env = env;
    item->next = NULL;

    enif_mutex_lock(queue->lock);

    if(queue->tail != NULL)
    {
        queue->tail->next = item;
    }

    queue->tail = item;

    if(queue->head == NULL)
    {
        queue->head = queue->tail;
    }

    enif_cond_signal(queue->cond);
    enif_mutex_unlock(queue->lock);

    return 1;
}

void queue_stop(queue_t *queue)
{
    enif_mutex_lock(queue->lock);
    queue->stopping = 1;
    enif_cond_signal(queue->cond);
    enif_mutex_unlock(queue->lock);
}

qitem_t *queue_pop(queue_t* queue)
{
    qitem_t* item;

    enif_mutex_lock(queue->lock);

    while(queue->head == NULL && !queue->stopping)
    {
        enif_cond_wait(queue->cond, queue->lock);
    }

    if (queue->stopping) {
        enif_mutex_unlock(queue->lock);
        return NULL;
    }
    
    item = queue->head;
    queue->head = item->next;
    item->next = NULL;

    if(queue->head == NULL)
    {
        queue->tail = NULL;
    }

    enif_mutex_unlock(queue->lock);
    return item;
}

static void*
thr_main(void* obj)
{
    async_state_t* state = (async_state_t*) obj;
    qitem_t* item;

    state->ref_env = enif_alloc_env();
    state->ref_cnt = 0;
    
    state->queue->stopping = 0;
    while((item = queue_pop(state->queue)) != NULL)
    {
        enif_send(NULL, &state->pid, item->env, item->term);
        enif_free_env(item->env);
        enif_free(item);

        if (state->ref_cnt == 0) {
            enif_clear_env(state->ref_env);
        }
    }

    return NULL;
}

async_state_t *async_start()
{
    async_state_t* state = (async_state_t*) enif_alloc(sizeof(async_state_t));
    if(state == NULL) return NULL;
    
    state->queue = queue_create();
    if(state->queue == NULL) goto error;

    state->opts = enif_thread_opts_create("thread_opts");
    if(enif_thread_create(
            "", &(state->qthread), thr_main, state, state->opts
        ) != 0)
    {
        goto error;
    }
    
    return state;

error:
    if(state->queue != NULL) queue_destroy(state->queue);
    enif_free(state->queue);
    DBG("Error starting async!");

    return NULL;
}

void async_stop(async_state_t *state)
{
    void* resp;
    
    queue_stop(state->queue);
    enif_thread_join(state->qthread, &resp);

    queue_destroy(state->queue);

    if (state->ref_cnt > 0) {
        enif_free_env(state->ref_env);
    }
    
    enif_thread_opts_destroy(state->opts);
    enif_free(state);
}


ErlNifEnv *async_env_acquire(async_state_t *state)
{
    enif_mutex_lock(state->queue->lock);
    return enif_alloc_env();
}

void async_env_release_and_send(async_state_t *state, ErlNifEnv *env, ERL_NIF_TERM term)
{
    enif_mutex_unlock(state->queue->lock);
    queue_push(state->queue, env, term);
}

void async_set_pid(async_state_t *state, ErlNifPid pid)
{
    state->pid = pid;
}

ERL_NIF_TERM *async_make_ref(async_state_t *state)
{
    ERL_NIF_TERM *r = (ERL_NIF_TERM *)enif_alloc(sizeof(ERL_NIF_TERM));
    *r = enif_make_ref(state->ref_env);
    state->ref_cnt++;
    return r;
}

ERL_NIF_TERM async_return_ref(async_state_t *state, ERL_NIF_TERM *refptr)
{
    // Copy the reference to the enif_send temp environment
    ERL_NIF_TERM ref_term = enif_make_copy(state->ref_env, *refptr);
    enif_free(refptr);
    state->ref_cnt--;
    return ref_term;
}
