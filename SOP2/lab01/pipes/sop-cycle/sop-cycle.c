#include "pipe-utils.h"

#define DEFAULT_N 3
#define BUFSIZE 11
#define LOWER_BOUND -10
#define UPPER_BOUND 15
#define INITIAL_MESSAGE 3
#define DEFAULT_DELAY 1e2
#define MILI_TO_NANO 1e6

#define STOP_CONDITION(num) ((num) > 999)

void usage(char *pname);
void empty_sighandler(int signo);
int check_stop_condition(long num);
void read_command_line_argument(int argc, char **argv, int *n);
void modify_message(long *pnum);
void read_write(char *buffer, int n, int rp, int wp);
void send_initial_message(int wp, char *buffer, int n);

int main(int argc, char **argv)
{
    if (sethandler(empty_sighandler, SIGINT))
        ERR("sethandler");
    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("sethandler");

    int n;
    read_command_line_argument(argc, argv, &n);

    pipe_t *pipes = malloc(n * sizeof(pipe_t));
    if (!pipes)
        ERR("malloc");
    create_pipes(pipes, n);

    char buffer[BUFSIZE];
    memset(buffer, 0, BUFSIZE);

    pid_t pid;
    for (int i = 0; i < n - 1; i++)
    {
        switch ((pid = fork()))
        {
        case 0: // child
            close_unused_pipes(pipes, n, i);
            read_write(buffer, BUFSIZE, pipes[i][READ], pipes[i + 1][WRITE]);
            close_pipe(pipes, i, READ);
            close_pipe(pipes, i + 1, WRITE);
            free(pipes);
            exit(EXIT_SUCCESS);
            break;
        case -1:
            ERR("fork");
            break;
        }
    }
    close_unused_pipes(pipes, n, n - 1);
    int rp = pipes[n - 1][READ];
    int wp = pipes[0][WRITE];

    send_initial_message(wp, buffer, BUFSIZE);
    read_write(buffer, BUFSIZE, rp, wp);

    close_pipe(pipes, n - 1, READ);
    close_pipe(pipes, 0, WRITE);
    free(pipes);
    while (wait(NULL) > 0)
        ;
    return EXIT_SUCCESS;
}

void empty_sighandler(int signo)
{
    UNUSED(signo);
}

void usage(char *pname)
{
    fprintf(stderr, "USAGE: %s [N >= 2]", pname);
    exit(EXIT_FAILURE);
}

void read_command_line_argument(int argc, char **argv, int *n)
{
    if (argc > 2)
        usage(argv[0]);
    *n = DEFAULT_N;
    if (argc == 2)
    {
        *n = atoi(argv[1]);
        if (*n <= 1)
            usage(argv[0]);
    }
}

int check_stop_condition(long num)
{
    return STOP_CONDITION(num);
}

void modify_message(long *pnum)
{
    *pnum += rand() % (UPPER_BOUND - LOWER_BOUND + 1) + LOWER_BOUND;
}

void read_write(char *buffer, int n, int rp, int wp)
{
    struct timespec delay;
    delay.tv_sec = 0;
    delay.tv_nsec = DEFAULT_DELAY * MILI_TO_NANO;

    srand((unsigned)time(NULL) * getpid());
    while (1)
    {
        ssize_t ret;
        errno = 0;
        if ((ret = read(rp, buffer, n)) < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
                break;
            else
                ERR("read");
        }
        if (ret == 0)
            break;
        fprintf(stderr, "[%d] received %s\n", getpid(), buffer);

        if (nanosleep(&delay, NULL) < 0 && errno == EINTR)
            break;

        errno = 0;
        long num = strtol(buffer, NULL, 10);
        if (errno == ERANGE || errno == EINVAL)
            ERR("strtol");

        if (check_stop_condition(num))
            break;
        modify_message(&num);
        snprintf(buffer, n, "%ld", num);

        fprintf(stderr, "[%d] writing %s\n", getpid(), buffer);
        errno = 0;
        if (write(wp, buffer, n) < 0)
        {
            if (errno == EAGAIN || errno == EINTR || errno == ESPIPE)
                break;
            else
                ERR("write");
        }
    }
}

void send_initial_message(int wp, char *buffer, int n)
{
    srand((unsigned)time(NULL) * getpid());
    long num = INITIAL_MESSAGE;
    snprintf(buffer, n, "%ld", num);
    if (write(wp, buffer, n) < 0)
        ERR("write");
    fprintf(stderr, "[%d] initial writing %s\n", getpid(), buffer);
}

