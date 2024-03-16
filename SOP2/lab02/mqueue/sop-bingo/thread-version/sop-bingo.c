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

#define UNUSED(x) ((void)(x))
#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define MAX_N 99
#define MIN_N 1
#define BINGO_IN "/bingo_in"
#define BINGO_OUT "/bingo_out"
#define PERM 0600
#define MSGMAX 10
#define MSGSIZE (sizeof(char))
#define WINNING_PRIO 1
#define NORMAL_PRIO 0

typedef void (*routinehandler_t)(int, siginfo_t *, void *);
typedef struct mq_attr mq_attr_t;
typedef struct sigevent sigevent_t;
typedef char buffer_t;
typedef unsigned int UINT;
typedef struct bingo_routine_args
{
    mqd_t pin;
    pthread_mutex_t *pmtx;
    int *pmessages_received;
} bingo_routine_args_t;

void usage(char *name);
void read_command_line_argument(int argc, char **argv, int *n);
void sethandler(routinehandler_t f, int sigNo);
void bingo_player(int player_index, mqd_t pin, mqd_t pout);
void bingo_server(mqd_t pout, bingo_routine_args_t args);
void bingo_threadroutine(union sigval sv);

int main(int argc, char **argv)
{
    int n;
    read_command_line_argument(argc, argv, &n);
    printf("n = %d\n", n);

    if (mq_unlink(BINGO_IN) < 0)
    {
        if (errno != ENOENT)
            ERR("mq_unlink");
    }
    if (mq_unlink(BINGO_OUT) < 0)
    {
        if (errno != ENOENT)
            ERR("mq_unlink");
    }

    mq_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = MSGMAX;
    attr.mq_msgsize = MSGSIZE;

    mqd_t pin, pout;

    pid_t pid;
    for (int i = 0; i < n; i++)
    {
        switch ((pid = fork()))
        {
        case -1:
            ERR("fork");
        case 0: // child
        {
            if ((pin = TEMP_FAILURE_RETRY(mq_open(BINGO_IN, O_WRONLY | O_CREAT, PERM, &attr))) < 0)
                ERR("mq_open");
            if ((pout = TEMP_FAILURE_RETRY(mq_open(BINGO_OUT, O_RDONLY | O_CREAT, PERM, &attr))) < 0)
                ERR("mq_open");

            bingo_player(i, pin, pout);

            if (mq_close(pin) < 0)
                ERR("mq_close");
            if (mq_close(pout) < 0)
                ERR("mq_close");

            exit(EXIT_SUCCESS);
        }
        }
    }

    if ((pin = TEMP_FAILURE_RETRY(mq_open(BINGO_IN, O_RDONLY | O_CREAT | O_NONBLOCK, PERM, &attr))) < 0)
        ERR("mq_open");
    if ((pout = TEMP_FAILURE_RETRY(mq_open(BINGO_OUT, O_WRONLY | O_CREAT, PERM, &attr))) < 0)
        ERR("mq_open");

    int messages_received = n;
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    bingo_routine_args_t args = {.pin = pin, .pmessages_received = &messages_received, .pmtx = &mtx};

    sigevent_t not ;
    not .sigev_notify = SIGEV_THREAD;
    not .sigev_notify_function = bingo_threadroutine;
    not .sigev_notify_attributes = NULL;
    not .sigev_value.sival_ptr = &args;
    if (mq_notify(pin, &not ) < 0)
        ERR("mq_notify");

    bingo_server(pout, args);

    while (wait(NULL) > 0)
        ;

    if (mq_close(pin) < 0)
        ERR("mq_close");
    if (mq_close(pout) < 0)
        ERR("mq_close");

    if (mq_unlink(BINGO_IN))
        ERR("mq_unlink");
    if (mq_unlink(BINGO_OUT))
        ERR("mq_unlink");

    return EXIT_SUCCESS;
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s %d <= n <= %d\n", name, MIN_N, MAX_N);
    exit(EXIT_FAILURE);
}

void read_command_line_argument(int argc, char **argv, int *n)
{
    if (argc != 2)
        usage(argv[0]);
    *n = atoi(argv[1]);
    if (*n < MIN_N || *n > MAX_N)
        usage(argv[0]);
}

void sethandler(routinehandler_t f, int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void bingo_player(int player_index, mqd_t pin, mqd_t pout)
{
    srand((unsigned)time(NULL) * getpid());

    int N = rand() % 10 + 1;
    buffer_t E = rand() % 10;
    buffer_t buffer;
    int ret;

    for (int i = 0; i < N; i++)
    {
        if ((ret = TEMP_FAILURE_RETRY(mq_receive(pout, &buffer, MSGMAX, NULL))) < 0)
            ERR("mq_receive");
        printf("[%d] I received %d from Bingo Server!\n", player_index, (int)buffer);
        if (E == buffer)
        {
            printf("[%d] I won with number %d!\n", player_index, (int)E);
            if ((ret = TEMP_FAILURE_RETRY(mq_send(pin, &E, MSGSIZE, WINNING_PRIO))) < 0)
                ERR("mq_send");
            return;
        }
    }
    printf("[%d] I'm leaving the game after %d rounds!\n", player_index, N);
    if ((ret = TEMP_FAILURE_RETRY(mq_send(pin, (buffer_t *)&player_index, MSGSIZE, NORMAL_PRIO))) < 0)
        ERR("mq_send");
}

void bingo_server(mqd_t pout, bingo_routine_args_t args)
{
    srand((unsigned)time(NULL) * getpid());
    buffer_t bingo;
    int ret;

    while (1)
    {
        pthread_mutex_lock(args.pmtx);
        if (*(args.pmessages_received) <= 0)
        {
            pthread_mutex_unlock(args.pmtx);
            break;
        }
        pthread_mutex_unlock(args.pmtx);

        bingo = rand() % 10 + 1;
        if ((ret = TEMP_FAILURE_RETRY(mq_send(pout, &bingo, MSGSIZE, NORMAL_PRIO))) < 0)
            ERR("mq_send");
        for (int t = 1; t > 0; t = sleep(t))
            ;
    }
    printf("Bingo Server: Terminating the gameplay...\n");
}

void bingo_threadroutine(union sigval sv)
{
    bingo_routine_args_t *args = (bingo_routine_args_t *)sv.sival_ptr;
    mqd_t pin = args->pin;

    UINT prio;
    int ret;
    buffer_t buffer;

    sigevent_t not ;
    not .sigev_notify = SIGEV_THREAD;
    not .sigev_notify_function = bingo_threadroutine;
    not .sigev_notify_attributes = NULL;
    not .sigev_value.sival_ptr = args;
    if (mq_notify(pin, &not ) < 0)
        ERR("mq_notify");

    while (1)
    {
        errno = 0;
        if ((ret = TEMP_FAILURE_RETRY(mq_receive(pin, &buffer, MSGSIZE, &prio))) < 0)
        {
            if (errno == EAGAIN)
                break;
            ERR("mq_receive");
        }
        if (prio == WINNING_PRIO)
        {
            printf("Bingo Server Routine: %d is a BINGO number!\n", (int)buffer);
        }
        else
        {
            printf("Bingo Server Routine: player %d has left the game!\n", (int)buffer);
        }
        pthread_mutex_lock(args->pmtx);
        *(args->pmessages_received) -= 1;
        pthread_mutex_unlock(args->pmtx);
    }
}