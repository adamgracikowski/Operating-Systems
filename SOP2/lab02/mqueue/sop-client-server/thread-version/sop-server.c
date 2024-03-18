#include "client-server-utils.h"

void sighandler(int sig);
void server_worker(union sigval sv);

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

    char from_names[OPERATION_COUNT][QUEUE_NAME_MAX];
    mqd_t from_queues[OPERATION_COUNT];

    sigevent_t not [OPERATION_COUNT];
    worker_args_t args[OPERATION_COUNT];

    for (int i = 0; i < OPERATION_COUNT; i++)
    {
        if (snprintf(from_names[i], QUEUE_NAME_MAX, "/%d_%c", server_pid, operation_code[i]) < 0)
            ERR("snprintf");
        if ((from_queues[i] = mq_open(from_names[i], O_RDONLY | O_CREAT | O_NONBLOCK, PERM, &from_attr)) < 0)
            ERR("mq_open");
        printf("Server: %s created with descriptor %d\n", from_names[i], from_queues[i]);

        args[i] = (worker_args_t){
            .from_queue = from_queues[i],
            .code = i,
            .to_attr = to_attr,
            .mask = mask};

        restore_notify_thread(from_queues[i], &not [i], server_worker, &args[i]);
    }

    while (sigsuspend(&old_mask) < 0)
    {
        if (should_exit)
        {
            printf("Server [%d]: I've received SIGINT!\n", server_pid);
            break;
        }
    }

    for (int i = 0; i < OPERATION_COUNT; i++)
    {
        if (mq_close(from_queues[i]) < 0)
            ERR("mq_close");
        printf("Server [%d]: I've closed %s!\n", server_pid, from_names[i]);
        if (mq_unlink(from_names[i]) < 0)
            ERR("mq_unlink");
        printf("Server [%d]: I've unlinked %s!\n", server_pid, from_names[i]);
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

void server_worker(union sigval sv)
{
    int ret;
    worker_args_t *args = (worker_args_t *)sv.sival_ptr;
    printf("Server Worker [%d]: Starting with descriptor %d\n", getpid(), args->from_queue);

    if (sigprocmask(SIG_BLOCK, &args->mask, NULL) < 0)
        ERR("sigprocmask");

    from_client_t from_data;
    to_client_t to_data;
    size_t from_length = sizeof(from_client_t);
    size_t to_length = sizeof(to_client_t);

    char to_name[QUEUE_NAME_MAX];
    mqd_t to_queue;

    sigevent_t not ;
    restore_notify_thread(args->from_queue, &not, server_worker, args);

    while (1)
    {
        errno = 0;
        if ((ret = mq_receive(args->from_queue, (char *)&from_data, from_length, NULL)) < 0)
        {
            if (errno == EAGAIN)
                break;
            ERR("mq_receive");
        }
        printf("Server Worker [%d]: I've received (%ld, %ld) from Client [%d].\n", getpid(),
               from_data.left_operand, from_data.right_operand, from_data.client_pid);

        create_client_queue_name(to_name, QUEUE_NAME_MAX, from_data.client_pid);
        if ((to_queue = mq_open(to_name, O_WRONLY | O_CREAT, &args->to_attr)) < 0)
            ERR("mq_open");
        printf("Server Worker [%d]: I've estabilished a connection with Client [%d] via %s.\n", getpid(), from_data.client_pid, to_name);

        to_data.result = operation[from_data.operation_code](from_data.left_operand, from_data.right_operand);

        if ((ret = mq_send(to_queue, (char *)&to_data, to_length, 0)) < 0)
            ERR("mq_receive");
        printf("Server Worker [%d]: I've sent %ld to Client [%d].\n", getpid(), to_data.result, from_data.client_pid);

        if (mq_close(to_queue) < 0)
            ERR("mq_close");
        printf("Server Worker [%d]: I've closed %s.\n", getpid(), to_name);
    }
    printf("Server Worker [%d]: I've finished my job.\n", getpid());
}
