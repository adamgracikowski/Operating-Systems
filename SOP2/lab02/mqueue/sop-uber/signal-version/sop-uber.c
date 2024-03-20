#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define UNUSED(x) ((void)(x))
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define MIN_N 1
#define MIN_T 5
#define MIN_UBER_DELAY 5e2
#define MAX_UBER_DELAY 2e3
#define MAX_COORD 1e3
#define FROM_MAXMSG 10
#define TO_MAXMSG 10
#define QUEUE_MAX_NAME 50
#define UBER_QUEUE_NAME "/uber_tasks"
#define PERM 0666
#define LOW_PRIO 0
#define HIGH_PRIO 1
#define MILI_TO_NANO 1e6
#define MILI 1e3
#define NANO 1e9

typedef void (*signalhandler_t)(int);
typedef void (*siginfohandler_t)(int, siginfo_t *, void *);
typedef void (*notifyhandler_t)(union sigval);
typedef struct mq_attr mq_attr_t;
typedef struct sigevent sigevent_t;
typedef struct timespec timespec_t;
typedef unsigned int UINT;

typedef struct position
{
    int x;
    int y;
} position_t;

typedef struct to_driver
{
    position_t start;
    position_t end;
} to_driver_t; // passed to drivers from uber

typedef struct from_driver
{
    int distance;
    pid_t pid;
} from_driver_t; // passed from drivers to uber

volatile sig_atomic_t received_alarm = 0;

void alarm_handler(int signo);
void usage(const char *name);
void parse_argv(int argc, char **argv, int *N, int *T);
void sethandler_siginfo(siginfohandler_t f, int signo);
void sethandler(signalhandler_t f, int signo);
void restore_notify_signal(mqd_t mq, int signo, void *args);
void milisleep(time_t miliseconds);
void prepare_attr(mq_attr_t *pattr, long msgsize, long maxmsg);
void create_driver_queue_name(char *name, size_t name_length, pid_t pid);
int calculate_distance(position_t *p1, position_t *p2);
int cover_distance(to_driver_t *to_driver, position_t *current);
void uber_handler(int signo, siginfo_t *siginfo, void *context);
int rand_coord();
void driver_work(mq_attr_t *from_attr, mq_attr_t *to_attr);
void create_drivers(pid_t *driver_pid, int N, mq_attr_t from_attr, mq_attr_t to_attr);

int main(int argc, char **argv)
{
    int N, T;
    parse_argv(argc, argv, &N, &T);
    printf("N = %d, T = %d\n", N, T);

    sethandler_siginfo(uber_handler, SIGRTMIN);
    sethandler(alarm_handler, SIGALRM);

    sigset_t mask, old_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    sigaddset(&mask, SIGRTMIN);
    if (sigprocmask(SIG_BLOCK, &mask, &old_mask) < 0)
        ERR("sigprocmask");

    mq_attr_t from_attr, to_attr;
    long from_length = sizeof(from_driver_t); // 'from' means driver -> uber
    long to_length = sizeof(to_driver_t);     // 'to' means uber -> driver

    prepare_attr(&to_attr, to_length, TO_MAXMSG);
    prepare_attr(&from_attr, from_length, FROM_MAXMSG);

    pid_t driver_pid[N];
    create_drivers(driver_pid, N, from_attr, to_attr);

    srand((unsigned)time(NULL) * getpid());
    mqd_t uber_queue;
    if ((uber_queue = mq_open(UBER_QUEUE_NAME, O_CREAT | O_WRONLY | O_NONBLOCK, PERM, &to_attr)) < 0)
        ERR("mq_open");

    char driver_name[N][QUEUE_MAX_NAME];
    mqd_t driver_queue[N];
    for (int i = 0; i < N; i++)
    {
        create_driver_queue_name(driver_name[i], QUEUE_MAX_NAME, driver_pid[i]);
        if ((driver_queue[i] = mq_open(driver_name[i], O_CREAT | O_RDONLY | O_NONBLOCK, PERM, &from_attr)) < 0)
            ERR("mq_open");
        restore_notify_signal(driver_queue[i], SIGRTMIN, &driver_queue[i]);
    }

    if (sigprocmask(SIG_SETMASK, &old_mask, NULL) < 0)
        ERR("sigprocmask");

    alarm(T);

    to_driver_t to_driver;
    int ret;
    while (1)
    {
        if (received_alarm)
        {
            printf("Uber: SIGALRM received!\n");
            break;
        }
        to_driver.start.x = rand_coord();
        to_driver.start.y = rand_coord();
        to_driver.end.x = rand_coord();
        to_driver.end.y = rand_coord();

        errno = 0;
        if ((ret = TEMP_FAILURE_RETRY(mq_send(uber_queue, (char *)&to_driver, to_length, LOW_PRIO))) < 0)
        {
            if (errno == EAGAIN)
                fprintf(stderr, "Uber: Queue full!\n");
            else
                ERR("mq_send");
        }
        time_t uber_delay = rand() % (int)(MAX_UBER_DELAY - MIN_UBER_DELAY + 1) + MIN_UBER_DELAY;
        milisleep(uber_delay);
    }

    memset(&to_driver, 0, sizeof(to_driver));
    UINT prio = HIGH_PRIO;
    for (int i = 0; i < N; i++)
    {
        if ((ret = mq_send(uber_queue, (char *)&to_driver, to_length, prio)) < 0)
            ERR("mq_send");
    }

    pid_t waited_pid;
    while ((waited_pid = wait(NULL)) > 0)
        printf("Uber: Waited for %d\n", waited_pid);

    for (int i = 0; i < N; i++)
    {
        if (mq_close(driver_queue[i]) < 0)
            ERR("mq_close");
        if (mq_unlink(driver_name[i]) < 0)
            ERR("mq_unlink");
        printf("Uber: Closing and unlinking %s\n", driver_name[i]);
    }
    if (mq_close(uber_queue) < 0)
        ERR("mq_close");
    if (mq_unlink(UBER_QUEUE_NAME) < 0)
        ERR("mq_unlink");
    printf("Uber: Closing and unlinking %s\n", UBER_QUEUE_NAME);

    return EXIT_SUCCESS;
}

void alarm_handler(int signo)
{
    UNUSED(signo);
    received_alarm = 1;
}

void usage(const char *name)
{
    fprintf(stderr, "USAGE: %s N T\n", name);
    fprintf(stderr, "N: 1 <= N - number of drivers\n");
    fprintf(stderr, "T: 5 <= T - simulation duration\n");
    exit(EXIT_FAILURE);
}

void parse_argv(int argc, char **argv, int *N, int *T)
{
    if (argc != 3)
        usage(argv[0]);
    *N = atoi(argv[1]);
    *T = atoi(argv[2]);
    if (*N < MIN_N || *T < MIN_T)
        usage(argv[0]);
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

void sethandler(signalhandler_t f, int signo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(signo, &act, NULL))
        ERR("sigaction");
}

void restore_notify_signal(mqd_t mq, int signo, void *args)
{
    sigevent_t not ;
    memset(&not, 0, sizeof(not ));
    not .sigev_notify = SIGEV_SIGNAL;
    not .sigev_signo = signo;
    not .sigev_value.sival_ptr = args;
    if (mq_notify(mq, &not ) < 0)
        ERR("mq_notify");
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

void prepare_attr(mq_attr_t *pattr, long msgsize, long maxmsg)
{
    memset(pattr, 0, sizeof(*pattr));
    pattr->mq_msgsize = msgsize;
    pattr->mq_maxmsg = maxmsg;
}

void create_driver_queue_name(char *name, size_t name_length, pid_t pid)
{
    if (snprintf(name, name_length, "/uber_results_%d", pid) < 0)
        ERR("snprintf");
}

int calculate_distance(position_t *p1, position_t *p2)
{
    return ABS(p1->x - p2->x) + ABS(p1->y - p2->y);
}

int cover_distance(to_driver_t *to_driver, position_t *current)
{
    int distance = calculate_distance(current, &to_driver->start) +
                   calculate_distance(&to_driver->start, &to_driver->end);
    current->x = to_driver->end.x;
    current->y = to_driver->end.y;
    return distance;
}

void uber_handler(int signo, siginfo_t *siginfo, void *context)
{
    UNUSED(signo);
    UNUSED(context);

    mqd_t driver_queue = *((mqd_t *)siginfo->si_value.sival_ptr);
    restore_notify_signal(driver_queue, SIGRTMIN, siginfo->si_value.sival_ptr);

    from_driver_t from_driver;
    long from_length = sizeof(from_driver_t);
    UINT prio;

    int ret;
    while (1)
    {
        errno = 0;
        if ((ret = mq_receive(driver_queue, (char *)&from_driver, from_length, &prio)) < 0)
        {
            if ((errno == EINTR && received_alarm) || errno == EAGAIN)
                break;
            ERR("mq_receive");
        }
        printf("Uber: Driver [%d] covered a distance of %d\n", from_driver.pid, from_driver.distance);
    }
}

int rand_coord()
{
    return rand() % (2 * (int)MAX_COORD + 1) - MAX_COORD;
}

void driver_work(mq_attr_t *from_attr, mq_attr_t *to_attr)
{
    pid_t pid = getpid();
    srand((unsigned)time(NULL) * pid);
    UINT prio;
    int ret;

    long from_length = sizeof(from_driver_t);
    long to_length = sizeof(to_driver_t);
    to_driver_t to_driver;
    from_driver_t from_driver;
    from_driver.pid = getpid();

    mqd_t driver_queue, uber_queue;
    char driver_name[QUEUE_MAX_NAME];
    create_driver_queue_name(driver_name, QUEUE_MAX_NAME, pid);
    if ((driver_queue = mq_open(driver_name, O_CREAT | O_WRONLY, PERM, from_attr)) < 0)
        ERR("mq_open");
    if ((uber_queue = mq_open(UBER_QUEUE_NAME, O_CREAT | O_RDONLY, PERM, to_attr)) < 0)
        ERR("mq_open");

    position_t current = {
        .x = rand_coord(),
        .y = rand_coord()};
    printf("Driver [%d]: Initial position (%d, %d)\n", getpid(), current.x, current.y);

    while (1)
    {
        if ((ret = mq_receive(uber_queue, (char *)&to_driver, to_length, &prio)) < 0)
            ERR("mq_receive");
        if (prio == HIGH_PRIO)
            break;

        printf("Driver [%d]: Received request for (%d, %d) -> (%d, %d)\n", getpid(),
               to_driver.start.x, to_driver.start.y, to_driver.end.x, to_driver.end.y);

        from_driver.distance = cover_distance(&to_driver, &current);
        milisleep((time_t)from_driver.distance);

        printf("Driver [%d]: Changed position to (%d, %d), covered distance %d\n", getpid(),
               current.x, current.y, from_driver.distance);

        if ((ret = mq_send(driver_queue, (char *)&from_driver, from_length, LOW_PRIO)) < 0)
            ERR("mq_send");

        printf("Driver [%d]: Sent distance %d to Uber\n", getpid(), from_driver.distance);
    }
    if (mq_close(uber_queue) < 0)
        ERR("mq_close");
    printf("Driver [%d]: Closing %s\n", getpid(), driver_name);
}

void create_drivers(pid_t *driver_pid, int N, mq_attr_t from_attr, mq_attr_t to_attr)
{
    for (int i = 0; i < N; i++)
    {
        switch ((driver_pid[i] = fork()))
        {
        case -1:
            ERR("fork");
        case 0: // child
            driver_work(&from_attr, &to_attr);
            exit(EXIT_SUCCESS);
        }
    }
}