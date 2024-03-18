#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE
#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>

#define UNUSED(x) ((void)(x))
#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define PERM 0666
#define QUEUE_NAME_MAX 15
#define TO_MAXMSG 10
#define FROM_MAXMSG 10
#define MILI_TO_NANO 1e6
#define SECOND_TO_NANO 1e9
#define OPERATION_COUNT 3

typedef long (*operation_t)(long, long);
typedef void (*sighandler_t)(int);
typedef void (*siginfohandler_t)(int, siginfo_t *, void *);
typedef void (*notifyhandler_t)(union sigval);
typedef struct mq_attr mq_attr_t;
typedef struct sigevent sigevent_t;
typedef struct timespec timespec_t;
typedef unsigned int UINT;

typedef struct from_client
{
    pid_t client_pid;
    long left_operand;
    long right_operand;
    int operation_code;
} from_client_t;

typedef struct to_client
{
    long result;
} to_client_t;

typedef struct worker_args
{
    mq_attr_t to_attr;
    mqd_t from_queue;
    sigset_t mask;
    int code;
} worker_args_t;

void prepare_attr(mq_attr_t *pattr, long msgsize, long maxmsg);
void create_client_queue_name(char *name, size_t name_length, pid_t pid);
long add(long left_operand, long right_operand);
long divide(long left_operand, long right_operand);
long modulo(long left_operand, long right_operand);
void prepare_delay(timespec_t *ts, time_t seconds, time_t nanoseconds);

void restore_notify_signal(mqd_t mq, sigevent_t * not, int signo, void *args);
void restore_notify_thread(mqd_t mq, sigevent_t * not, notifyhandler_t routine, void *args);
void sethandler(sighandler_t f, int signo);
void sethandler_siginfo(siginfohandler_t f, int signo);

operation_t operation[OPERATION_COUNT] = {add, divide, modulo};
char operation_code[OPERATION_COUNT] = {'s', 'd', 'm'};

void sethandler(sighandler_t f, int signo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(signo, &act, NULL))
        ERR("sigaction");
}

void sethandler_siginfo(siginfohandler_t f, int signo)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = f;
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(signo, &sa, NULL) < 0)
        ERR("sigaction");
}

void restore_notify_signal(mqd_t mq, sigevent_t * not, int signo, void *args)
{
    memset(not, 0, sizeof(*not ));
    not ->sigev_notify = SIGEV_SIGNAL;
    not ->sigev_signo = signo;
    not ->sigev_value.sival_ptr = args;
    if (mq_notify(mq, not ) < 0)
        ERR("mq_notify");
}

void restore_notify_thread(mqd_t mq, sigevent_t * not, notifyhandler_t routine, void *args)
{
    memset(not, 0, sizeof(sigevent_t));
    not ->sigev_notify = SIGEV_THREAD;
    not ->sigev_notify_function = routine;
    not ->sigev_notify_attributes = NULL;
    not ->sigev_value.sival_ptr = args;
    if (mq_notify(mq, not ) < 0)
        ERR("mq_notify");
}

void create_client_queue_name(char *name, size_t name_length, pid_t pid)
{
    if (snprintf(name, name_length, "/%d", pid) < 0)
        ERR("snprintf");
}

void prepare_attr(mq_attr_t *pattr, long msgsize, long maxmsg)
{
    memset(pattr, 0, sizeof(*pattr));
    pattr->mq_msgsize = msgsize;
    pattr->mq_maxmsg = maxmsg;
}

void prepare_delay(timespec_t *ts, time_t seconds, time_t nanoseconds)
{
    if (clock_gettime(CLOCK_REALTIME, ts) < 0)
        ERR("clock_gettime");
    ts->tv_sec += seconds;
    ts->tv_nsec += nanoseconds;
    if (ts->tv_nsec >= 1 * SECOND_TO_NANO)
    {
        ts->tv_sec++;
        ts->tv_nsec -= 1 * SECOND_TO_NANO;
    }
}

long add(long left_operand, long right_operand)
{
    return left_operand + right_operand;
}

long divide(long left_operand, long right_operand)
{
    if (right_operand == 0)
        return LONG_MAX;
    return left_operand / right_operand;
}

long modulo(long left_operand, long right_operand)
{
    if (right_operand == 0)
        return LONG_MAX;
    return left_operand % right_operand;
}