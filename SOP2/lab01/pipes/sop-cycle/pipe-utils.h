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

void close_pipe(pipe_t *pipes, int i, int j) {
    if (close(pipes[i][j]) < 0)
        ERR("close");
    fprintf(stderr, "[%d] closing pipes[%d][%d]\n", getpid(), i, j);
}

void close_unused_pipes(pipe_t *pipes, int n, int i) {
    // closing read-ends
    for (int j = 0; j < i; j++)
    {
        if (close(pipes[j][READ]) < 0)
            ERR("close");
        fprintf(stderr, "[%d] closing pipes[%d][%d]\n", getpid(), j, READ);
    }
    for (int j = i + 1; j < n; j++)
    {
        if (close(pipes[j][READ]) < 0)
            ERR("close");
        fprintf(stderr, "[%d] closing pipes[%d][%d]\n", getpid(), j, READ);
    }

    i = (i + 1) % n;

    // closing write-ends
    for (int j = 0; j < i; j++)
    {
        if (close(pipes[j][WRITE]) < 0)
            ERR("close");
        fprintf(stderr, "[%d] closing pipes[%d][%d]\n", getpid(), j, WRITE);
    }
    for (int j = i + 1; j < n; j++)
    {
        if (close(pipes[j][WRITE]) < 0)
            ERR("close");
        fprintf(stderr, "[%d] closing pipes[%d][%d]\n", getpid(), j, WRITE);
    }
}

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

int sethandler(sighandler_t f, int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        return -1;
    return 0;
}