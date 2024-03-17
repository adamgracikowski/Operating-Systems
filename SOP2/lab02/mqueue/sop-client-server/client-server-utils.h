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
#define OPERATION_COUNT 3

typedef struct mq_attr mq_attr_t;
typedef struct sigevent sigevent_t;
typedef unsigned int UINT;
typedef long (*operation_t)(long, long);
typedef void (*sighandler_t)(int);
typedef void (*notifyhandler_t)(union sigval);

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

void prepare_attr(mq_attr_t *pattr, long msgsize, long maxmsg);
void sethandler(sighandler_t f, int sigNo);
void create_client_queue_name(char *name, size_t name_length, pid_t pid);
long add(long left_operand, long right_operand);
long divide(long left_operand, long right_operand);
long modulo(long left_operand, long right_operand);

operation_t operation[OPERATION_COUNT] = {add, divide, modulo};
char operation_code[OPERATION_COUNT] = {'s', 'd', 'm'};

void sethandler(sighandler_t f, int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void prepare_attr(mq_attr_t *pattr, long msgsize, long maxmsg)
{
    memset(pattr, 0, sizeof(*pattr));
    pattr->mq_msgsize = msgsize;
    pattr->mq_maxmsg = maxmsg;
}

void create_client_queue_name(char *name, size_t name_length, pid_t pid)
{
    if (snprintf(name, name_length, "/%d", pid) < 0)
        ERR("snprintf");
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
