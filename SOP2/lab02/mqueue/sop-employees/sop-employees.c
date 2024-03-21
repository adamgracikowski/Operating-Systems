#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <mqueue.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define UNUSED(x) ((void)(x))
#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define MAX_QUEUE_NAME 256
#define TASK_QUEUE_NAME "/task_queue_%d"
#define RESULT_QUEUE_NAME "/result_queue_%d_%d"

#define MAX_MSGS 10
#define MIN_WORKERS 2
#define MAX_WORKERS 20
#define MIN_TIME 1e2
#define MAX_TIME 5e3
#define MIN_DELAY 5e2
#define MAX_DELAY 2e3
#define MILI_TO_NANO 1e6
#define MILI 1e3
#define NANO 1e9
#define PERM 0666

typedef void (*signalhandler_t)(int);
typedef void (*siginfohandler_t)(int, siginfo_t *, void *);
typedef void (*notifyhandler_t)(union sigval);
typedef struct mq_attr mq_attr_t;
typedef struct sigevent sigevent_t;
typedef struct timespec timespec_t;
typedef unsigned int UINT;

typedef struct to_employee_t
{
    float x;
    float y;
} to_employee_t;

typedef struct from_employee_t
{
    float result;
    pid_t pid;
} from_employee_t;

volatile sig_atomic_t should_exit = 0;

void usage(const char *name);
void parse_argv(int argc, char **argv, int *N, int *T1, int *T2);
void should_exit_handler(int signo);
void create_result_queue_name(char *buffer, int buffer_length, pid_t pid, int idx);
void create_server_queue_name(char *buffer, int buffer_length, pid_t pid);
void milisleep(time_t miliseconds);
void sethandler(signalhandler_t f, int signo);
void restore_notify_thread(mqd_t mq, sigevent_t * not, notifyhandler_t routine, void *args);
void server_routine(union sigval sv);
void prepare_attr(mq_attr_t *pattr, long msgsize, long maxmsg);
float draw_number();

int main(int argc, char **argv)
{
    int N, T1, T2;
    parse_argv(argc, argv, &N, &T1, &T2);
    printf("N = %d, T1 = %d, T2 = %d\n", N, T1, T2);

    sethandler(should_exit_handler, SIGINT);

    sigset_t mask, old_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    if (sigprocmask(SIG_BLOCK, &mask, &old_mask) < 0)
        ERR("sigprocmask");

    printf("Server is starting...\n");

    char to_name[MAX_QUEUE_NAME];
    char from_names[N][MAX_QUEUE_NAME];
    for (int i = 0; i < N; i++)
    {
        create_result_queue_name(from_names[i], MAX_QUEUE_NAME, getpid(), i);
        mq_unlink(from_names[i]);
    }
    create_server_queue_name(to_name, MAX_QUEUE_NAME, getpid());
    mq_unlink(to_name);

    from_employee_t from_data;
    to_employee_t to_data;
    int from_length = sizeof(from_employee_t);
    int to_length = sizeof(to_employee_t);

    mq_attr_t to_attr, from_attr;
    prepare_attr(&to_attr, to_length, MAX_MSGS);
    prepare_attr(&from_attr, from_length, MAX_MSGS);

    pid_t pid;
    for (int i = 0; i < N; i++)
    {
        switch ((pid = fork()))
        {
            case -1:
                ERR("fork");
            case 0:  // child
                srand(getpid());
                from_data.pid = getpid();
                printf("[%d]: Employee ready!\n", getpid());

                mqd_t to_queue, from_queue;
                if ((to_queue = mq_open(to_name, O_CREAT | O_RDONLY, PERM, &to_attr)) < 0)
                    ERR("mq_open");
                if ((from_queue = mq_open(from_names[i], O_CREAT | O_WRONLY, PERM, &from_attr)) < 0)
                    ERR("mq_open");

                UINT prio;
                while (1)
                {
                    if (mq_receive(to_queue, (char *)&to_data, to_length, &prio) < 0)
                        ERR("mq_receive");
                    if (prio == 1)
                    {
                        printf("[%d]: Server told me that it's time to finish.\n", getpid());
                        break;
                    }
                    printf("[%d]: Received task [%f, %f]\n", getpid(), to_data.x, to_data.y);
                    
                    int delay = rand() % (int)(MAX_DELAY - MIN_DELAY + 1) + MIN_DELAY;
                    milisleep((time_t)delay);
                    
                    from_data.result = to_data.x + to_data.y;
                    if (mq_send(from_queue, (char *)&from_data, from_length, 0) < 0)
                        ERR("mq_send");
                    printf("[%d]: Result sent %f\n", getpid(), from_data.result);
                }

                printf("[%d]: Worker Exits!\n", getpid());

                if (mq_close(to_queue) < 0)
                    ERR("mq_close");
                if (mq_close(from_queue) < 0)
                    ERR("mq_close");
                exit(EXIT_SUCCESS);
        }
    }

    mqd_t from_queues[N], to_queue;
    for (int i = 0; i < N; i++)
    {
        if ((from_queues[i] = mq_open(from_names[i], O_CREAT | O_RDONLY | O_NONBLOCK, PERM, &from_attr)) < 0)
            ERR("mq_open");
        sigevent_t not ;
        restore_notify_thread(from_queues[i], &not, server_routine, &from_queues[i]);
    }
    if ((to_queue = mq_open(to_name, O_CREAT | O_WRONLY | O_NONBLOCK, PERM, &to_attr)) < 0)
        ERR("mq_open");

    if (sigprocmask(SIG_SETMASK, &old_mask, NULL) < 0)
        ERR("sigprocmask");

    int ret;
    while (1)
    {
        if (should_exit)
        {
            printf("Server: Received SIGINT. No new tasks queued.\n");
            break;
        }
        
        int delay = rand() % (T2 - T1 + 1) + T1;
        milisleep((time_t)delay);
        
        to_data.x = draw_number();
        to_data.y = draw_number();

        errno = 0;
        if ((ret = mq_send(to_queue, (char *)&to_data, to_length, 0)) < 0)
        {
            if (should_exit)
            {
                printf("Server: Received SIGINT. No new tasks queued.\n");
                break;
            }
            if (errno == EAGAIN)
            {
                fprintf(stderr, "Server: Queue is full!\n");
                continue;
            }
            else
                ERR("mq_send");
        }
        printf("New task queued: [%f, %f]\n", to_data.x, to_data.y);
    }

    memset(&to_data, 0, sizeof(to_data));
    UINT prio = 1;
    for (int i = 0; i < N;)
    {
        errno = 0;
        if ((ret = mq_send(to_queue, (char *)&to_data, to_length, prio)) < 0)
        {
            if (errno == EAGAIN)
            {
                fprintf(stderr, "Server: Queue is full!\n");
                milisleep(100);
                continue;
            }
            else
                ERR("mq_send");
        }
        i++;
    }

    while (wait(NULL) > 0)
        ;

    printf("Server: All child processes have finished.\n");

    for (int i = 0; i < N; i++)
    {
        if (mq_close(from_queues[i]) < 0)
            ERR("mq_close");
        if (mq_unlink(from_names[i]) < 0)
            ERR("mq_unlink");
    }
    if (mq_close(to_queue) < 0)
        ERR("mq_close");
    if (mq_unlink(to_name) < 0)
        ERR("mq_unlink");

    return EXIT_SUCCESS;
}

void usage(const char *name)
{
    fprintf(stderr, "USAGE: %s N T1 T2\n", name);
    fprintf(stderr, "N: %d <= N <= %d - number of workers\n", (int)MIN_WORKERS, (int)MAX_WORKERS);
    fprintf(stderr, "T1, T2: %d <= T1 < T2 <= %d - time range for spawning new tasks\n", (int)MIN_TIME, (int)MAX_TIME);
    exit(EXIT_FAILURE);
}

void parse_argv(int argc, char **argv, int *N, int *T1, int *T2)
{
    if (argc != 4)
        usage(argv[0]);
    *N = atoi(argv[1]);
    *T1 = atoi(argv[2]);
    *T2 = atoi(argv[3]);
    if (*N < MIN_WORKERS || *N > MAX_WORKERS)
        usage(argv[0]);
    if (*T1 < MIN_TIME || *T2 < MIN_TIME || *T1 > MAX_TIME || *T2 > MAX_TIME || *T1 >= *T2)
        usage(argv[0]);
}

void should_exit_handler(int signo)
{
    UNUSED(signo);
    should_exit = 1;
}

void create_result_queue_name(char *buffer, int buffer_length, pid_t pid, int idx)
{
    if (snprintf(buffer, buffer_length, RESULT_QUEUE_NAME, pid, idx) < 0)
        ERR("snprintf");
}

void create_server_queue_name(char *buffer, int buffer_length, pid_t pid)
{
    if (snprintf(buffer, buffer_length, TASK_QUEUE_NAME, pid) < 0)
        ERR("snprintf");
}

void milisleep(time_t miliseconds)
{
    timespec_t requested, remaining;
    memset(&requested, 0, sizeof(requested));
    requested.tv_sec = miliseconds / MILI;
    requested.tv_nsec = (miliseconds % (int)MILI) * MILI_TO_NANO;

    errno = 0;
    while (nanosleep(&requested, &remaining) < 0)
    {
        if (errno != EINTR)
            ERR("nanosleep");
        requested = remaining;
    }
}

void sethandler(signalhandler_t f, int signo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(signo, &act, NULL))
        ERR("sigaction");
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

void server_routine(union sigval sv)
{
    mqd_t from_queue = *((mqd_t *)sv.sival_ptr);
    sigevent_t not ;
    restore_notify_thread(from_queue, &not, server_routine, sv.sival_ptr);

    from_employee_t from_data;
    int from_length = sizeof(from_employee_t);
    UINT prio;

    int ret;
    while (1)
    {
        errno = 0;
        if ((ret = mq_receive(from_queue, (char *)&from_data, from_length, &prio)) < 0)
        {
            if (errno == EAGAIN)
                break;
            ERR("mq_receive");
        }
        printf("Server: Received result from employee %d: %f\n", from_data.pid, from_data.result);
    }
}

void prepare_attr(mq_attr_t *pattr, long msgsize, long maxmsg)
{
    memset(pattr, 0, sizeof(*pattr));
    pattr->mq_msgsize = msgsize;
    pattr->mq_maxmsg = maxmsg;
}

float draw_number()
{
    int r = rand();
    float rf = (float)r / RAND_MAX;
    return rf * 100.0;
}
