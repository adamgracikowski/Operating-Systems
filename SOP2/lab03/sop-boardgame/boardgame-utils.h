#define _GNU_SOURCE

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define UNUSED(x) ((void)(x))
#define ERR(source) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define MIN_N 4
#define MAX_N 20
#define SHM_NAME_MAX 50
#define PERM 0666
#define SHM_SIZE(n) (sizeof(shm_data_t) + (n) * sizeof(char))

typedef struct timespec timespec_t;
typedef struct
{
    int *pexit;
    pthread_mutex_t *pmtx;
    sigset_t mask;
} sigthread_args_t;

typedef struct
{
    pthread_mutex_t board_mutex;
    pthread_mutex_t counter_mutex;
    int counter;
    int n;
    char board[];
} shm_data_t;

void create_shm_name(char *buffer, int buffer_length, pid_t pid)
{
    if (snprintf(buffer, buffer_length, "/%d-board", pid) < 0)
        ERR("snprintf");
}

void print_board(char *board, int n)
{
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
            printf("%d ", board[i * n + j]);
        putchar('\n');
    }
    putchar('\n');
}

void sop_mutexattr_shared(pthread_mutexattr_t *attr)
{
    if (pthread_mutexattr_init(attr))
        ERR("pthread_mutexattr_init");
    if (pthread_mutexattr_setpshared(attr, PTHREAD_PROCESS_SHARED))
        ERR("pthread_mutexattr_setpshared");
    if (pthread_mutexattr_setrobust(attr, PTHREAD_MUTEX_ROBUST))
        ERR("pthread_mutexattr_setrobust");
}

void sop_condattr_shared(pthread_condattr_t *attr)
{
    if (pthread_condattr_init(attr))
        ERR("pthread_condattr_init");
    if (pthread_condattr_setpshared(attr, PTHREAD_PROCESS_SHARED))
        ERR("pthread_condattr_setpshared");
}

void sop_barrierattr_shared(pthread_barrierattr_t *attr)
{
    if (pthread_barrierattr_init(attr))
        ERR("pthread_barrierattr_init");
    if (pthread_barrierattr_setpshared(attr, PTHREAD_PROCESS_SHARED))
        ERR("pthread_barrierattr_setpshared");
}

void sop_mutex_sharedlock(pthread_mutex_t *mtx)
{
    int ret;
    if ((ret = pthread_mutex_lock(mtx)))
    {
        if (ret == EOWNERDEAD)
            pthread_mutex_consistent(mtx);
        else
            ERR("pthread_mutex_lock");
    }
}

void sop_mutex_sharedunlock(pthread_mutex_t *mtx)
{
    if (pthread_mutex_unlock(mtx))
        ERR("pthread_mutex_unlock");
}

void *sigthread_routine(void *void_args)
{
    sigthread_args_t *args = (sigthread_args_t *)void_args;

    int signo;
    while (1)
    {
        if (sigwait(&args->mask, &signo))
            ERR("sigwait");
        switch (signo)
        {
        case SIGINT:
            // add your logic
            pthread_mutex_lock(args->pmtx);
            *(args->pexit) = 1;
            pthread_mutex_unlock(args->pmtx);
            return NULL;
        default:
            ERR("unknown signal");
        }
    }
    return NULL;
}

void sop_block_signals(sigset_t *mask, sigset_t *old_mask, int *signo, int n)
{
    if (sigemptyset(mask))
        ERR("sigemptyset");
    for (int i = 0; i < n; i++)
    {
        if (sigaddset(mask, signo[i]))
            ERR("sigaddset");
    }
    if (pthread_sigmask(SIG_BLOCK, mask, old_mask))
        ERR("pthread_sigmask");
}

void sop_shm_unsubscribe(shm_data_t *shm_ptr)
{
    sop_mutex_sharedlock(&shm_ptr->counter_mutex);
    if (--shm_ptr->counter == 0)
    {
        printf("[%d]: Destoying shared memory components.\n", getpid());
        pthread_mutex_destroy(&shm_ptr->board_mutex);
        sop_mutex_sharedunlock(&shm_ptr->counter_mutex);
        pthread_mutex_destroy(&shm_ptr->counter_mutex);
    }
    else
    {
        sop_mutex_sharedunlock(&shm_ptr->counter_mutex);
    }
}

void sop_shm_subscribe(shm_data_t *shm_ptr)
{
    sop_mutex_sharedlock(&shm_ptr->counter_mutex);
    shm_ptr->counter++;
    sop_mutex_sharedunlock(&shm_ptr->counter_mutex);
}