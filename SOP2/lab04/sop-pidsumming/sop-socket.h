#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/select.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define UNUSED(x) (void)(x)

#define PID_LENGTH 11

typedef void (*signalhandler_t)(int);

int sop_sethandler(signalhandler_t f, int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        return -1;
    return 0;
}

int sop_setnonblock(int fd)
{
    int oldflags = fcntl(fd, F_GETFL, 0);
    if (oldflags == -1)
        return -1;
    oldflags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, oldflags);
}

int sop_make_sockstream()
{
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        ERR("socket");
    return sock;
}

struct sockaddr_in sop_make_address(char *address, char *port)
{
    int ret;
    struct sockaddr_in addr;
    struct addrinfo *result;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    if ((ret = getaddrinfo(address, port, &hints, &result)))
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        exit(EXIT_FAILURE);
    }
    addr = *(struct sockaddr_in *)(result->ai_addr);
    freeaddrinfo(result);
    return addr;
}

int sop_connect_sockstream(char *name, char *port)
{
    struct sockaddr_in addr;
    int socketfd;
    socketfd = sop_make_sockstream();
    addr = sop_make_address(name, port);
    if (connect(socketfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
        ERR("connect");
    return socketfd;
}

int sop_bind_sockstream(uint16_t port, int backlog)
{
    struct sockaddr_in addr;
    int socketfd, t = 1;
    socketfd = sop_make_sockstream();
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)))
        ERR("setsockopt");
    if (bind(socketfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        ERR("bind");
    if (listen(socketfd, backlog) < 0)
        ERR("listen");
    return socketfd;
}

int sop_accept_client(int server_socket)
{
    int nfd;
    if ((nfd = TEMP_FAILURE_RETRY(accept(server_socket, NULL, NULL))) < 0)
    {
        if (EAGAIN == errno || EWOULDBLOCK == errno)
            return -1;
        ERR("accept");
    }
    return nfd;
}

ssize_t sop_bulk_read(int fd, char *buf, size_t count)
{
    int c;
    size_t len = 0;

    fd_set read_fds;

    do
    {
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);

        int ready = select(fd + 1, &read_fds, NULL, NULL, NULL);
        if (ready < 0)
            return -1;
        if (!FD_ISSET(fd, &read_fds))
            continue;

        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (0 == c)
            return len;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t sop_bulk_write(int fd, char *buf, size_t count)
{
    int c;
    size_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}