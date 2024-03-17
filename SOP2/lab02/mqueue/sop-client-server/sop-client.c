#include "client-server-utils.h"

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s SERVER_QUEUE_NAME\n", name);
    exit(EXIT_FAILURE);
}

int validate_from_name(char *from_name)
{
    char code = from_name[strlen(from_name) - 1];
    for (int i = 0; i < OPERATION_COUNT; i++)
    {
        if (operation_code[i] == code)
            return i;
    }
    ERR("invalid server queue name");
}

int main(int argc, char **argv)
{
    int ret;

    if (argc != 2)
        usage(argv[0]);
    char *from_name = argv[1];

    int code = validate_from_name(from_name);

    pid_t pid = getpid();
    printf("Client [%d]: I'm going to connect with Server via %s.\n", pid, from_name);

    char to_name[QUEUE_NAME_MAX];
    create_client_queue_name(to_name, QUEUE_NAME_MAX, pid);

    from_client_t from_data;
    to_client_t to_data;
    size_t from_length = sizeof(from_client_t);
    size_t to_length = sizeof(to_client_t);

    mq_attr_t to_attr, from_attr;
    prepare_attr(&to_attr, to_length, TO_MAXMSG);
    prepare_attr(&from_attr, from_length, FROM_MAXMSG);

    mqd_t to_queue, from_queue;
    if ((from_queue = mq_open(from_name, O_WRONLY | O_CREAT, PERM, &from_attr)) < 0)
        ERR("mq_open");
    printf("Client [%d]: I've opened %s.\n", pid, from_name);
    if ((to_queue = mq_open(to_name, O_RDONLY | O_CREAT, PERM, &to_attr)) < 0)
        ERR("mq_open");
    printf("Client [%d]: I've opened %s.\n", pid, to_name);

    struct timespec ts;
    long left_operand, right_operand;
    while (scanf("%ld %ld", &left_operand, &right_operand) == 2)
    {
        from_data = (from_client_t){
            .client_pid = pid,
            .left_operand = left_operand,
            .right_operand = right_operand,
            .operation_code = code};

        printf("Client [%d]: I've sent (%ld, %ld) via %s.\n", pid, left_operand, right_operand, from_name);

        if (clock_gettime(CLOCK_REALTIME, &ts) < 0)
            ERR("clock_gettime");
        ts.tv_sec += 3;

        errno = 0;
        if ((ret = mq_timedsend(from_queue, (char *)&from_data, from_length, 0, &ts)) < 0)
        {
            if (errno == ETIMEDOUT)
            {
                printf("Client [%d]: I've been waiting too long to send (%ld, %ld) via %s.\n", pid, left_operand, right_operand, from_name);
                continue;
            }
            ERR("mq_timedsend");
        }

        if (clock_gettime(CLOCK_REALTIME, &ts) < 0)
            ERR("clock_gettime");
        ts.tv_sec += 1;

        errno = 0;
        if ((ret = mq_timedreceive(to_queue, (char *)&to_data, to_length, NULL, &ts)) < 0)
        {
            if (errno == ETIMEDOUT)
            {
                printf("Client [%d]: I've been waiting too long for the result!\n", pid);
                break;
            }
            ERR("mq_receive");
        }
        printf("Client [%d]: I've received %ld via %s.\n", pid, to_data.result, to_name);
    }

    if (mq_close(from_queue) < 0)
        ERR("mq_close");
    if (mq_close(to_queue) < 0)
        ERR("mq_close");
    if (mq_unlink(to_name) < 0)
        ERR("mq_unlink");
    printf("Client %d: I've closed %s.\n", pid, from_name);
    printf("Client %d: I've closed and unlinked %s.\n", pid, to_name);
    return EXIT_SUCCESS;
}