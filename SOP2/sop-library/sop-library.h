#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <mqueue.h>

#define UNUSED(x) (void)(x)
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))
#define TRUE 1
#define FALSE 0

/************** Random numbers **************/

#define NEXT_DOUBLE(a, b) (((double)rand() / RAND_MAX) * ((b) - (a)) + (a))
#define NEXT_INT(a, b) (rand() % ((b) - (a) + 1))
    
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int sop_randint(int a, int b)
{
    if (a > b)
        ERR("sop_randint");
    return rand() % (a - b + 1) + a;
}

long sop_randlong(long a, long b)
{
    if (a > b)
        ERR("sop_randint");
    return rand() % (a - b + 1) + a;
}

double sop_randdouble(double a, double b)
{
    if (a > b)
        ERR("sop_randint");
    return (rand() % RAND_MAX) * (b - a + 1) + a;
}

float sop_randfloat(float a, float b)
{
    if (a > b)
        ERR("sop_randint");
    return (rand() % RAND_MAX) * (b - a + 1) + a;
}

char sop_randletter(int upper)
{
    return rand() % ('A' - 'Z' + 1) + 'A' + (upper ? 0 : 'a' - 'A');
}

/************** Signals **************/

typedef void (*signalhandler_t)(int);
typedef void (*siginfohandler_t)(int, siginfo_t *, void *);
typedef struct timespec timespec_t;
typedef struct
{
    sigset_t mask;
} sigthread_args_t;

void sop_setsiginfohandler(siginfohandler_t f, int signo)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = f;
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(signo, &sa, NULL) < 0)
        ERR("sigaction");
}

void sop_setsignalhandler(signalhandler_t f, int signo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(signo, &act, NULL))
        ERR("sigaction");
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
        ERR("sigprocmask");
}

void sop_sigthread_routine(void *void_args)
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
            break;
        case SIGUSR1:
            // add your logic
            break;
        case SIGUSR2:
            // add your logic
            break;
        default:
            ERR("unknown signal");
        }
    }
}

/************** Sleeping **************/

#define MILI_TO_NANO 1e6

void sop_milisleep(time_t miliseconds)
{
    timespec_t requested, remaining;
    memset(&requested, 0, sizeof(requested));
    requested = {
        .tv_sec = miliseconds / (int)1e3,
        .tv_nsec = (miliseconds % (int)1e3) * (int)1e6};

    errno = 0;
    while (nanosleep(&requested, &remaining) < 0)
    {
        if (errno != EINTR)
            ERR("sop_milisleep");
        requested = remaining;
    }
}

void sop_nanosleep(time_t nanoseconds)
{
    timespec_t requested, remaining;
    memset(&requested, 0, sizeof(requested));
    requested = {
        .tv_sec = nanoseconds / (int)1e9,
        .tv_nsec = nanoseconds % (int)1e9};

    errno = 0;
    while (nanosleep(&requested, &remaining) < 0)
    {
        if (errno != EINTR)
            ERR("sop_nanosleep");
        requested = remaining;
    }
}

void sop_sleep(unsigned seconds)
{
    unsigned remaining;
    while ((remaining = sleep(seconds)) > 0)
        seconds = remaining;
}

/************** Synchronization **************/

#define SHM_SIZE(n, type) (sizeof(shm_data_t) + (n) * sizeof(type))

typedef struct shm_data_t
{

} shm_data_t;

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
