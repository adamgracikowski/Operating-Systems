#pragma once
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))
#define UNUSED(x) ((void)(x))
#define SWAP(x, y)        \
    do                    \
    {                     \
        typeof(x) _x = x; \
        typeof(y) _y = y; \
        x = _y;           \
        y = _x;           \
    } while (0)

#define READ 0
#define WRITE 1

typedef unsigned int UINT;
typedef int pipe_t[2];
typedef void (*sighandler_t)(int);

void copy_pipe(int *from, int *to)
{
    to[0] = from[0];
    to[1] = from[1];
}

void create_pipes(pipe_t *pipes, int n)
{
    int fd[2];
    for (int i = 0; i < n; i++)
    {
        if (pipe(fd) < 0)
            ERR("pipe");
        copy_pipe(fd, pipes[i]);
    }
}

void close_pipe(int fd)
{
    if (close(fd) < 0)
        ERR("close");
}

void close_pipes_all(int *pipes, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (close(pipes[i]) < 0)
            ERR("close");
    }
}

void close_pipes_except(pipe_t *pipes, int n, int except_idx, int pipe_end)
{
    for (int i = 0; i < n; i++)
    {
        if (i != except_idx)
        {
            if (close(pipes[i][READ]) < 0)
                ERR("close");
            if (close(pipes[i][WRITE]) < 0)
                ERR("close");
        }
    }
    if (close(pipes[except_idx][WRITE - pipe_end]) < 0)
        ERR("close");
}

void close_pipes_all_one_end(pipe_t *pipes, int n, int one_end)
{
    for (int i = 0; i < n; i++)
    {
        if (close(pipes[i][one_end]) < 0)
            ERR("close");
    }
}

int sethandler(sighandler_t f, int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        return -1;
    return 0;
}