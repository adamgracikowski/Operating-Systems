#include "socket-utils.h"

#define BACKLOG_SIZE 10
#define MAX_CLIENT_COUNT 4
#define MAX_EVENTS 10

#define NAME_OFFSET 0
#define NAME_SIZE 64
#define MESSAGE_OFFSET NAME_SIZE
#define MESSAGE_SIZE 448
#define BUFF_SIZE (NAME_SIZE + MESSAGE_SIZE)

#define UNUSED(x) (void)(x)
#define EMPTY_KEY "\0"

typedef struct client_t
{
    int fd;
    char name[NAME_SIZE];
    int request_counter;
} client_t;

volatile sig_atomic_t do_work = 1;

void usage(char *pname);
void parse_argv(int argc, char **argv, uint16_t *port, char **key);
void sigint_handler(int sig);
void sop_setnonblock(int fd);
void client_reset(client_t *client);
int find_index(client_t *clients);
void server_accept_client(int server_socket, client_t *clients, char *buffer, char *key, int epoll_descriptor);
void server_disconnect_client(client_t *client, int epoll_descriptor);
void server_shutdown(client_t *clients, int epoll_descriptor);
void server_broadcast(client_t *clients, char *buffer, int from, int epoll_descriptor);
void server_work(int server_socket, char *key, sigset_t oldmask);

int main(int argc, char **argv)
{
    uint16_t port;
    char *key;
    parse_argv(argc, argv, &port, &key);
    fprintf(stderr, "Server: port = %hu, key = %s\n", port, key);

    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("sethandler");
    if (sethandler(sigint_handler, SIGINT))
        ERR("sethandler");

    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    if (sigprocmask(SIG_BLOCK, &mask, &oldmask))
        ERR("sigprocmask");

    int server_socket = bind_tcp_socket(port, BACKLOG_SIZE);
    sop_setnonblock(server_socket);
    server_work(server_socket, key, oldmask);

    if (sigprocmask(SIG_UNBLOCK, &mask, NULL))
        ERR("sigprocmask");

    if (TEMP_FAILURE_RETRY(close(server_socket)) < 0)
        ERR("close");
    fprintf(stderr, "Server: Terminating.\n");
    return EXIT_SUCCESS;
}

void usage(char *pname)
{
    fprintf(stderr, "USAGE: %s port key\n", pname);
    exit(EXIT_FAILURE);
}

void parse_argv(int argc, char **argv, uint16_t *port, char **key)
{
    *key = EMPTY_KEY;
    if (argc < 2 || argc > 3)
        usage(argv[0]);
    if (sscanf(argv[1], "%hu", port) != 1)
        usage(argv[0]);
    if (argc > 2)
        *key = argv[2];
}

void sigint_handler(int sig)
{
    UNUSED(sig);
    do_work = 0;
}

void sop_setnonblock(int fd)
{
    int oldflags = fcntl(fd, F_GETFL, 0);
    if (oldflags == -1)
        ERR("fcntl");
    oldflags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, oldflags) == -1)
        ERR("fcntl");
}

void client_reset(client_t *client)
{
    client->fd = -1;
    client->request_counter = 0;
    memset(client->name, 0, NAME_SIZE);
}

int find_index(client_t *clients)
{
    for (int i = 1; i <= MAX_CLIENT_COUNT; i++)
    {
        if (clients[i].fd == -1)
            return i;
    }
    return -1;
}

void server_accept_client(int server_socket, client_t *clients, char *buffer, char *key, int epoll_descriptor)
{
    int ret;
    int client_socket = add_new_client(server_socket);
    if (client_socket == -1)
        return;

    fprintf(stderr, "Server: A new client is trying to connect.\n");
    int client_idx = find_index(clients);
    if (client_idx == -1)
    {
        fprintf(stderr, "Server: Not enough space for a new client!\n");
        if (TEMP_FAILURE_RETRY(close(client_socket)) < 0)
            ERR("close");
        return;
    }

    memset(buffer, 0, BUFF_SIZE);
    if ((ret = bulk_read(client_socket, buffer, BUFF_SIZE)) <= 0)
    {
        fprintf(stderr, "Server: Client discarded.\n");
        if (TEMP_FAILURE_RETRY(close(client_socket)) < 0)
            ERR("close");
        return;
    }

    char *client_name = buffer;
    char *client_key = buffer + MESSAGE_OFFSET;

    fprintf(stderr, "Server: %s is a new client with a key = %s\n", client_name, key);

    if (strcmp(key, client_key) != 0)
    {
        fprintf(stderr, "Server: %s has an incorrect key.\n", client_name);
        fprintf(stderr, "Server: %s has been rejected\n", client_name);
        if (TEMP_FAILURE_RETRY(close(client_socket)) < 0)
            ERR("close");
        return;
    }

    fprintf(stderr, "Server: %s has a correct key.\n", client_name);
    if ((ret = bulk_write(client_socket, buffer, BUFF_SIZE)) < 0)
    {
        if (errno == EPIPE)
        {
            fprintf(stderr, "Server: %s has disconnected\n", client_name);
            if (TEMP_FAILURE_RETRY(close(client_socket)) < 0)
                ERR("close");
        }
        else
            ERR("bulk_write");
    }
    else
    {
        clients[client_idx].fd = client_socket;
        strncpy(clients[client_idx].name, client_name, NAME_SIZE);

        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.ptr = &clients[client_idx];
        if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, client_socket, &event) == -1)
            ERR("epoll_ctl");
    }
}

void server_disconnect_client(client_t *client, int epoll_descriptor)
{
    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_DEL, client->fd, NULL) == -1)
        ERR("epoll_ctl");
    if (TEMP_FAILURE_RETRY(close(client->fd)) < 0)
        ERR("close");
    client->fd = -1;
}

void server_shutdown(client_t *clients, int epoll_descriptor)
{
    for (int i = 1; i <= MAX_CLIENT_COUNT; i++)
    {
        if (clients[i].fd != -1 && TEMP_FAILURE_RETRY(close(clients[i].fd)) < 0)
            ERR("close");
    }
    if (TEMP_FAILURE_RETRY(close(epoll_descriptor)) < 0)
        ERR("close");
}

void server_broadcast(client_t *clients, char *buffer, int from, int epoll_descriptor)
{
    int ret;
    char *from_name = buffer;
    for (int i = 1; i <= MAX_CLIENT_COUNT; i++)
    {
        int fd = clients[i].fd;
        if (fd >= 0 && fd != from)
        {
            if ((ret = bulk_write(fd, buffer, BUFF_SIZE)) < 0)
            {
                if (errno == EPIPE)
                {
                    fprintf(stderr, "Server: %s has disconnected\n", from_name);
                    server_disconnect_client(&clients[i], epoll_descriptor);
                }
                else
                    ERR("bulk_write");
            }
        }
    }
}

void server_work(int server_socket, char *key, sigset_t oldmask)
{
    int ret;
    client_t clients[MAX_CLIENT_COUNT + 1];
    for (int i = 0; i <= MAX_CLIENT_COUNT; i++)
        client_reset(&clients[i]);

    struct epoll_event event, events[MAX_EVENTS];

    int epoll_descriptor;
    if ((epoll_descriptor = epoll_create1(0)) < 0)
        ERR("epoll_create:");

    clients[0].fd = server_socket;
    event.events = EPOLLIN;
    event.data.ptr = &clients[0];
    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, server_socket, &event) == -1)
        ERR("epoll_ctl");

    int nfds;
    char buffer[BUFF_SIZE];
    memset(buffer, 0, BUFF_SIZE);

    while (do_work)
    {
        if ((nfds = epoll_pwait(epoll_descriptor, events, MAX_EVENTS, -1, &oldmask)) <= 0)
        {
            if (errno == EINTR && !do_work)
                break;
            ERR("epoll_pwait");
        }
        for (int i = 0; i < nfds && do_work; i++)
        {
            client_t *client = (client_t *)events[i].data.ptr;
            int fd = client->fd;
            if (fd == server_socket)
            {
                if (events[i].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP))
                {
                    fprintf(stderr, "Server: Unexpected error with server socket!\n");
                    do_work = 0;
                    continue;
                }
                else if (events[i].events & EPOLLIN)
                    server_accept_client(server_socket, clients, buffer, key, epoll_descriptor);
                continue;
            }

            if (events[i].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP))
            {
                fprintf(stderr, "Server: %s has disconnected\n", client->name);
                server_disconnect_client(client, epoll_descriptor);
                continue;
            }
            if (events[i].events & EPOLLIN)
            {
                memset(buffer, 0, BUFF_SIZE);
                if ((ret = bulk_read(fd, buffer, BUFF_SIZE)) <= 0)
                {
                    fprintf(stderr, "Server: %s has disconnected\n", client->name);
                    server_disconnect_client(client, epoll_descriptor);
                    continue;
                }
                else
                {
                    fprintf(stderr, "%s: %s\n", buffer, buffer + MESSAGE_OFFSET);
                    server_broadcast(clients, buffer, fd, epoll_descriptor);
                }
            }
        }
    }
    server_shutdown(clients, epoll_descriptor);
}