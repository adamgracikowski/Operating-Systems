#include "sop-socket.h"

#define BACKLOG 3
#define MAX_EVENTS 16

volatile sig_atomic_t do_work = 1;

void sigint_handler(int sig);
void usage(char *pname);
int16_t evaluate_response(char *pid);
void work(int server_socket, sigset_t oldmask);

int main(int argc, char **argv)
{
    if (argc != 2)
        usage(argv[0]);

    uint16_t port = (uint16_t)atoi(argv[1]);

    if (sop_sethandler(SIG_IGN, SIGPIPE))
        ERR("Seting SIGPIPE:");
    if (sop_sethandler(sigint_handler, SIGINT))
        ERR("Seting SIGINT:");

    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    int server_socket = sop_bind_sockstream(port, BACKLOG);
    if (sop_setnonblock(server_socket) == -1)
        ERR("sop_setnonblock");

    work(server_socket, oldmask);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    if (TEMP_FAILURE_RETRY(close(server_socket)) < 0)
        ERR("close");
    fprintf(stderr, "[%d]: Server has terminated.\n", getpid());
    return EXIT_SUCCESS;
}

void sigint_handler(int sig)
{
    UNUSED(sig);
    do_work = 0;
}

void usage(char *pname)
{
    printf("USAGE: %s port\n", pname);
    exit(EXIT_FAILURE);
}

int16_t evaluate_response(char *pid)
{
    int16_t response = 0;
    for (char *c = pid; *c; c++)
    {
        response += (int16_t)(*c - '0');
    }
    return response;
}

void work(int server_socket, sigset_t oldmask)
{
    int epoll_fd;
    if ((epoll_fd = epoll_create1(0)) < 0)
        ERR("epoll_create:");

    struct epoll_event event, events[MAX_EVENTS];
    event.data.fd = server_socket;
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) < 0)
        ERR("epoll_ctl");

    int nfds;
    char buffer[PID_LENGTH];
    memset(buffer, 0, PID_LENGTH);
    ssize_t size;

    while (do_work)
    {
        if ((nfds = epoll_pwait(epoll_fd, events, MAX_EVENTS, -1, &oldmask)) > 0)
        {
            for (int i = 0; i < nfds; i++)
            {
                int fd = events[i].data.fd;
                if (fd == server_socket)
                {
                    int client_socket = sop_accept_client(server_socket);
                    sop_setnonblock(client_socket);
                    event.data.fd = client_socket;
                    event.events = EPOLLIN;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) < 0)
                        ERR("epoll_ctl");
                    
                    printf("[%d]: Server added %d to epoll\n", getpid(), client_socket);
                }
                else
                {
                    if ((size = sop_bulk_read(fd, buffer, PID_LENGTH)) < 0)
                        ERR("sop_bulk_read:");
                    else if (size == PID_LENGTH)
                    {
                        int16_t response = htons(evaluate_response(buffer));
                        fprintf(stderr, "[%d]: Server evaluating response for %d\n", getpid(), fd);
                        if (sop_bulk_write(fd, (char *)&response, sizeof(int16_t)) < 0 && errno != EPIPE)
                            ERR("write:");
                    }
                    else if (size == 0)
                    {
                        printf("[%d]: Server removed %d from epoll\n", getpid(), fd);
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) < 0)
                            ERR("epoll_ctl");
                        if (TEMP_FAILURE_RETRY(close(fd)) < 0)
                            ERR("close");
                        printf("[%d]: Server closed %d\n", getpid(), fd);
                    }
                }
            }
        }
        else
        {
            if (errno == EINTR)
                continue;
            ERR("epoll_pwait");
        }
    }
    if (TEMP_FAILURE_RETRY(close(epoll_fd)) < 0)
        ERR("close");
}