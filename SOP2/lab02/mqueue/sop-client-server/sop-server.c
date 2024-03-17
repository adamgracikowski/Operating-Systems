#include "client-server-utils.h"

void sighandler(int sig);
void server_worker(pid_t server_pid, int code, mq_attr_t to_attr, mq_attr_t from_attr, sigset_t oldmask);

volatile sig_atomic_t should_exit = 0;

int main(void)
{
    sethandler(sighandler, SIGINT);

    sigset_t mask, old_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    if (sigprocmask(SIG_BLOCK, &mask, &old_mask) < 0)
        ERR("sigprocmask");

    pid_t server_pid = getpid();

    size_t from_length = sizeof(from_client_t);
    size_t to_length = sizeof(to_client_t);

    mq_attr_t to_attr, from_attr;
    prepare_attr(&to_attr, to_length, TO_MAXMSG);
    prepare_attr(&from_attr, from_length, FROM_MAXMSG);

    pid_t pid;
    for (int i = 0; i < OPERATION_COUNT; i++)
    {
        switch ((pid = fork()))
        {
        case -1:
            ERR("fork");
        case 0:
            server_worker(server_pid, i, to_attr, from_attr, old_mask);
            exit(EXIT_SUCCESS);
        }
    }

    while (sigsuspend(&old_mask) < 0)
    {
        if (should_exit)
        {
            printf("Server [%d]: I've received SIGINT!\n", server_pid);
            break;
        }
    }

    while (wait(NULL) > 0)
    {
        printf("Server [%d]: I've successfully waited for my Worker.\n", server_pid);
    }

    if (sigprocmask(SIG_SETMASK, &old_mask, NULL) < 0)
        ERR("sigprocmask");

    return EXIT_SUCCESS;
}

void sighandler(int sig)
{
    UNUSED(sig);
    should_exit = 1;
}

void server_worker(pid_t server_pid, int code, mq_attr_t to_attr, mq_attr_t from_attr, sigset_t oldmask)
{
    int ret;
    from_client_t from_data;
    to_client_t to_data;
    size_t from_length = sizeof(from_client_t);
    size_t to_length = sizeof(to_client_t);

    char to_name[QUEUE_NAME_MAX];
    char from_name[QUEUE_NAME_MAX];
    if (snprintf(from_name, QUEUE_NAME_MAX, "/%d_%c", server_pid, operation_code[code]) < 0)
        ERR("snprintf");

    mqd_t to_queue, from_queue;

    if ((from_queue = mq_open(from_name, O_RDONLY | O_CREAT, PERM, &from_attr)) < 0)
        ERR("mq_open");
    printf("Worker [%d]: I've opened %s.\n", getpid(), from_name);

    if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0)
        ERR("sigprocmask");

    while (!should_exit)
    {
        errno = 0;
        if ((ret = mq_receive(from_queue, (char *)&from_data, from_length, NULL)) < 0)
        {
            if (should_exit)
            {
                printf("Worker [%d]: I've received SIGINT!\n", getpid());
                break;
            }
            ERR("mq_receive");
        }
        printf("Server Worker [%d]: I've received (%ld, %ld) from Client [%d].\n", getpid(),
               from_data.left_operand, from_data.right_operand, from_data.client_pid);

        create_client_queue_name(to_name, QUEUE_NAME_MAX, from_data.client_pid);
        if (should_exit)
        {
            printf("Worker [%d]: I've received SIGINT!\n", getpid());
            break;
        }
        if ((to_queue = mq_open(to_name, O_WRONLY | O_CREAT, &to_attr)) < 0)
        {
            if (should_exit)
            {
                printf("Worker [%d]: I've received SIGINT!\n", getpid());
                break;
            }
            ERR("mq_open");
        }
        printf("Server Worker [%d]: I've estabilished a connection with Client [%d] via %s.\n", getpid(), from_data.client_pid, to_name);

        to_data.result = operation[from_data.operation_code](from_data.left_operand, from_data.right_operand);

        if ((ret = mq_send(to_queue, (char *)&to_data, to_length, 0)) < 0)
        {
            if (should_exit)
            {
                printf("Worker [%d]: I've received SIGINT!\n", getpid());
                break;
            }
            ERR("mq_receive");
        }
        printf("Server Worker [%d]: I've sent %ld to Client [%d].\n", getpid(), to_data.result, from_data.client_pid);

        if (mq_close(to_queue) < 0)
            ERR("mq_close");
        printf("Server Worker [%d]: I've closed %s.\n", getpid(), to_name);
    }
    if (mq_close(from_queue) < 0)
        ERR("mq_close");
    printf("Server Worker [%d]: I've closed %s.\n", getpid(), from_name);
    if (mq_unlink(from_name) < 0)
        ERR("mq_unlink");
    printf("Server Worker [%d]: I've unlinked %s\n", getpid(), from_name);
}