# Sockets:
## Functions:
```c
/* Basic Theory:
Sockets are a method of IPC that allow data to be exchanged between applications,
either on the same host (computer) or on different hosts connected by a network.

In a typical client-server scenario, applications communicate using sockets as follows:
- Each application creates a socket. A socket is the “apparatus” that allows communication, and both applications require one.
- The server binds its socket to a well-known address (name) so that clients can
locate it.

Stream sockets (SOCK_STREAM) provide a reliable, bidirectional, byte-stream communication channel. A stream socket is similar to using a pair of pipes to allow bidirectional communication between two applications, with the difference that (Internet domain) sockets permit communication over a network.

Datagram sockets (SOCK_DGRAM) allow data to be exchanged in the form of messages called datagrams. With datagram sockets, message boundaries are preserved, but data transmission is not reliable. Messages may arrive out of order, be duplicated, or not arrive at all.

Datagram sockets are an example of the more generic concept of a connectionless socket. Unlike a stream socket, a datagram socket doesn’t need to be connected to another socket in order to be used.

In the Internet domain, datagram sockets employ the User Datagram Protocol
(UDP), and stream sockets (usually) employ the Transmission Control Protocol (TCP).

Active and passive sockets:
- By default, a socket that has been created using socket() is active. An active socket can be used in a connect() call to establish a connection to a passive socket. This is referred to as performing an active open.
- A passive socket (also called a listening socket) is one that has been marked to allow incoming connections by calling listen(). Accepting an incoming connection is referred to as performing a passive open.

Using connect() with Datagram Sockets:
Even though datagram sockets are connectionless, the connect() system call serves a purpose when applied to datagram sockets. Calling connect() on a datagram socket causes the kernel to record a particular address as this socket’s peer. After a datagram socket has been connected:
- Datagrams can be sent through the socket using write() (or send()) and are automatically sent to the same peer socket. As with sendto(), each write() call results in a separate datagram.
- Only datagrams sent by the peer socket may be read on the socket.

Fundamentals of TCP/IP networks:
A networking protocol is a set of rules defining how information is to be transmitted across a network. Networking protocols are generally organized as a series of layers, with each layer building on the layer below it to add features that are made available to higher layers.

One of the notions that lends great power and flexibility to protocol layering is transparency — each protocol layer shields higher layers from the operation and complexity of lower layers.

The most notable difference between the two versions is that IPv4 identifies subnets and hosts using 32-bit addresses, while IPv6 uses 128-bit addresses, thus providing a much larger range of addresses to be assigned to hosts.

IP is described as a connectionless protocol, since it doesn’t provide the notion of a
virtual circuit connecting two hosts. IP is also an unreliable protocol: it makes a
“best effort” to transmit datagrams from the sender to the receiver, but doesn’t
guarantee that packets will arrive in the order they were transmitted, that they
won’t be duplicated, or even that they will arrive at all.

An IP address consists of two parts: a network ID, which specifies the network on
which a host resides, and a host ID, which identifies the host within that network.
*/

#include <sys/socket.h>
int socket(int socket_family, int socket_type, int protocol);
/* Desc:
    - creates an endpoint for communication and returns a file descriptor that refers to that endpoint.
    - protocol should be specified as 0 for most common use cases.
    - on success, a file descriptor for the new socket is returned.
    - on error, -1 is returned and errno is set to indicate the error.
    
Common values for socket_family:
- AF_INET - IPv4 Internet protocol, 
            address format: 32-bit IPv4 address + 16-bit port number,
            address structure: sockaddr_in.
- AF_INET6 - IPv6 Internet protocol, 
             address format: 128-bit IPv6 address + 16-bit port number,
             address structure: sockaddr_in6
- AF_UNIX - Local communication (within kernel),
            address format: pathname
            address structure: sockaddr_un

Common values for socket_type:
- SOCK_STREAM - reliable delivery, 
                connection-based, 
                no message boundaries are preserved (stream of bytes),
                two-way connection,
                must be in a connected state before any data may be sent or received on it.
- SOCK_DGRAM - unreliable messages of a fixed maximum length,
               message boundaries are preserved (datagrams),
               connectionless.
- SOCK_RAW - raw network protocol access.
- SOCK_NONBLOCK - using this flag saves extra calls to fcntl() to achieve the same result.
*/

#include <unistd.h>
int close(int fd);
/* Desc:
    - the usual way of terminating a stream socket connection is to call close().
    - if multiple file descriptors refer to the same socket, then the connection is terminated when all of the descriptors are closed.
*/

#include <sys/socket.h>
int shutdown(int sockfd, int how);
/* Desc:
    - causes all or part of a full-duplex connection on the socket associated with sockfd to be shut down.
    - if how is SHUT_RD, further receptions will be disallowed.
    - if how is SHUT_WR, further transmissions will be disallowed.
    - if how is SHUT_RDWR, further receptions and transmissions will be disallowed.
    - on success, 0 is returned.
    - on error, -1 is returned, and errno is set to indicate the error.
*/

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
/* Desc:
    - binds a socket to an address.
    - assigns the address specified by addr to the socket referred to by the file descriptor sockfd.
    - usually a server employs this call to bind its socket to a well-known address so that client can locate the socket.
    - traditionally, this operation is called "assigning a name to a socket”.
    - on success returns 0, otherwise -1 and sets errno to indicate the error.
    - for an Internet domain socket, the server could omit the call to bind() and simply call listen(), which causes the kernel to choose an ephemeral port for that socket.
    - afterwards the server can use getsockname() to retrieve the address of its socket.
*/

int listen(int sockfd, int backlog);
/* Desc:
    - marks the socket referred to by sockfd as a passive socket (a socket that will be used to accept incoming connection requests using accept()).
    - the sockfd argument is a file descriptor that refers to a socket of type SOCK_STREAM.
    - the backlog argument defines the maximum length to which the queue of pending connections for sockfd may grow.
    - if a connection request arrives when the queue is full, the client may receive an error with an indication of ECONNREFUSED.
    - EADDRINUSE means that another socket is already listening on the same port.
*/

int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
/* Desc:
    - accepts a connection from a peer application on a listening stream socket, and optionally returns the address of the peer socket.
    - used with SOCK_STREAM.
    - if no pending connections are present on the queue, and the socket is not marked as nonblocking, accept() blocks the caller until a connection is present.
    - if the socket is marked nonblocking and no pending connections are present on the queue, accept() fails with the error EAGAIN or EWOULDBLOCK.
    - in order to be notified of incoming connections on a socket, you can use select(), poll(), or epoll().
    - on success returns a file descriptor for the accepted socket (a nonnegative integer).
    - on error -1 is returned and errno is set to indicate the error.
*/

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
/* Desc:
    - estabilishes a connection with another socket.
    - connects the socket referred to by the file descriptor sockfd to the address specified by addr. The addrlen argument specifies the size of addr.
    - on success returns 0, otherwise -1 and sets errno to indicate the error.
    - if the socket sockfd is of type SOCK_DGRAM, then addr is the address to which datagrams are sent by default, and the only address from which datagrams are received.
    - if the socket is of type SOCK_STREAM, this call attempts to make a connection to the socket that is bound to the address specified by addr.
    - on error -1 is returned and errno is set to indicate the error.
    - if connect() fails, consider the state of the socket as unspecified.
    - portable applications should close the socket and create a new one for reconnecting.
*/

// "Base class" for addresses:
struct sockaddr {
   sa_family_t sa_family;
   char        sa_data[14];
}
// IPv4:
struct sockaddr_in {
   sa_family_t    sin_family; /* address family: AF_INET */
   in_port_t      sin_port;   /* port in network byte order */
   struct in_addr sin_addr;   /* internet address */
};
struct in_addr {
   uint32_t       s_addr;     /* address in network byte order */
};

// IPv6:
struct sockaddr_in6 {
   sa_family_t     sin6_family;   /* AF_INET6 */
   in_port_t       sin6_port;     /* port number */
   uint32_t        sin6_flowinfo; /* IPv6 flow information */
   struct in6_addr sin6_addr;     /* IPv6 address */
   uint32_t        sin6_scope_id; /* Scope ID (new in Linux 2.4) */
};
struct in6_addr {
   unsigned char   s6_addr[16];   /* IPv6 address */
};

// Unix Domain Socket Addresses:
struct sockaddr_un {
 sa_family_t sun_family; /* Always AF_UNIX */
 char sun_path[108]; /* Null-terminated socket pathname */
};

/* Sending and receiving messages:
Socket I/O can be performed using the conventional read() and write() system calls.
By default, these system calls block if the I/O operation can't be completed immediately.
*/

ssize_t send(int sockfd, const void* buf, size_t len, int flags);
ssize_t sendto(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
/* Desc:
    - used to transmit a message to another socket.
    - may be used only when the socket is in a connected state (so that the intended recipient is known).
    - with a zero flags argument, send() is equivalent to write().
    - if the message is too long to pass atomically through the underlying protocol, the error EMSGSIZE is returned, and the message is not transmitted.
    - on success returns number of bytes sent, otherwise returns -1 and sets the errno to indicate the error.
    - if sendto() is used on a connection-mode (SOCK_STREAM, SOCK_SEQPACKET) socket, the arguments dest_addr and addrlen are ignored (and the error EISCONN may be returned when they are not NULL and 0), and the error ENOTCONN is returned when the socket was not actually connected.
    - otherwise, the address of the target is given by dest_addr with addrlen specifying its size.
    - when writing onto a connection-oriented socket that has been shut down (by the local or the remote end) SIGPIPE is sent to the writing process and EPIPE is returned.
    - when the message does not fit into the send buffer of the socket, send() normally blocks, unless the socket has been placed in nonblocking I/O mode.
    - in nonblocking mode it would fail with the error EAGAIN or EWOULDBLOCK in this case.
    - the select() call may be used to determine when it is possible to send more data.
*/

ssize_t recv(int sockfd, void* buf, size_t len, int flags);
ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
/* Desc:
    - used to receive messages from a socket.
    - may be used to receive data on both connectionless and connection-oriented sockets.
    - the only difference between recv() and read() is the presence of flags.
    - recv(sockfd, buf, len, flags) is equivalent to recvfrom(sockfd, buf, len, flags, NULL, NULL).
    - returs the length of the message on successful completion.
    - if a message is too long to fit in the supplied buffer, excess bytes may be discarded depending on the type of socket the message is received from.
    - if no messages are available at the socket, the receive calls wait for a message to arrive, unless the socket is nonblocking.
    - if src_addr is not NULL, and the underlying protocol provides the source address of the message, that source address is placed in the buffer pointed to by src_addr.
    - if the caller is not interested in the source address, src_addr and addrlen should be specified as NULL.
    - when a stream socket peer has performed an orderly shutdown, the return value will be 0 (the traditional "end-of-file" return).
*/

// Bytes Reordering:
#include <arpa/inet.h>
uint32_t htonl(uint32_t hostlong);
/* Desc:
    - converts the unsigned integer hostlong from host byte order to network byte order.
*/
uint16_t htons(uint16_t hostshort);
/* Desc:
    - converts the unsigned short integer hostshort from host byte order to network byte order.
*/
uint32_t ntohl(uint32_t netlong);
/* Desc:
    - converts the unsigned integer netlong from network byte order to host byte order.
*/
uint16_t ntohs(uint16_t netshort);
/* Desc:
    - converts the unsigned short integer netshort from network byte order to host byte order.
*/

// Address Translation:
#include <sys/socket.h>
#include <netdb.h>
int getaddrinfo(const char* nodename, const char* servname,
    const struct addrinfo* hints, struct addrinfo** res);
 /* Desc:
    - translates the name of a service location (for example, a host name)
      and/or a service name and returns a set of socket addresses
      and associated information to be used in creating a socket with which
      to address the specified service.
    - allocates and initializes a linked list of addrinfo structures,
      one for each network address that matches node and service, 
      subject to any restrictions imposed by hints, and returns a pointer 
      to the start of the list in res.
    - the ai_next field of each structure contains a pointer to the next structure
      on the list, or a null pointer if it is the last structure on the list.
*/

void freeaddrinfo(struct addrinfo *ai);
/* Desc:
    - frees one or more addrinfo structures returned by getaddrinfo(), 
      along with any additional storage associated with those structures.
*/

struct addrinfo {
   int              ai_flags;
   int              ai_family;
   int              ai_socktype;
   int              ai_protocol;
   socklen_t        ai_addrlen;
   struct sockaddr *ai_addr;
   char            *ai_canonname;
   struct addrinfo *ai_next;
};
/* Desc:
    - ai_family - specifies the desired address family for the returned addresses.
                  Valid values for this field include AF_INET and AF_INET6.
                  The value AF_UNSPEC indicates that getaddrinfo() should return socket
                  addresses for any address family (either IPv4 or IPv6, for example)
                  that can be used with node and service.
   - ai_socktype - specifies the preferred socket type, for example SOCK_STREAM 
                   or SOCK_DGRAM. Specifying 0 in this field indicates that socket
                   addresses of any type can be returned by getaddrinfo().
*/

#include <arpa/inet.h>
const char *inet_ntop(int af, const void* src, char* dst, socklen_t size);
/* Desc:
    - converts the network address structure src in the af address family 
      into a character string.
    - the resulting string is copied to the buffer pointed to by dst, 
      which must be a non-null pointer.  
    - the caller specifies the number of bytes available in this buffer
      in the argument size.
*/

#include <sys/socket.h>
int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen);
int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen);
/* Desc:
    - manipulate options for the socket referred to by the file descriptor sockfd.
    - to manipulate options at the sockets API level, level is specified as SOL_SOCKET.
    - on success, 0 is returned for the standard options.
    - on error, -1 is returned, and errno is set to indicate the error.
*/

#include <sys/socket.h>
int getsockname(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
/* Desc:
    - returns the current address to which the socket sockfd is bound, in the buffer pointed to by addr.
    - the addrlen argument should be initialized to indicate the amount of space (in bytes) pointed to by addr.
    - on return it contains the actual size of the socket address.
    - on success, 0 is returned for the standard options.
    - on error, -1 is returned, and errno is set to indicate the error.
*/
```

# I/O Multiplexing:

## `select()` & `pselect()`:
```c
#include <sys/select.h>
typedef /* ... */ fd_set;
/* Desc:
       - a structure type that can represent a set of file descriptors.
       - according to POSIX, the maximum number of file descriptors 
         in an fd_set structure is the value of the macro FD_SETSIZE.
*/

void FD_ZERO(fd_set *set);
/* Desc:
    - clears (removes all file descriptors from) set.
    - should be employed as the first step in initializing a file descriptor set.
*/

void FD_SET(int fd, fd_set *set);
/* Desc:
    - adds the file descriptor fd to set.
    - adding a file descriptor that is already present does not produce an error.
*/

void FD_CLR(int fd, fd_set *set);
/* Desc:
    - removes the file descriptor fd from set.
    - removing a file descriptor that is not present does not produce an error.
*/

int  FD_ISSET(int fd, fd_set *set);
/* Desc:
    - used to test if a file descriptor is still present in a set.
    - usually invoked in a loop after select() or pselect().
*/

int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);
/* Desc:
    - nfds - should be set to the highest-numbered file descriptor in any of the three sets, plus 1.
    - readfds - the file descriptors in this set are watched to see if they are ready for reading.
                A file descriptor is ready for reading if a read operation will not block.
                In particular, a file descriptor is also ready on EOF.
    - writefds - the file descriptors in this set are watched to see if they are ready for writing.
                 A file descriptor is ready for writing if a write operation will not block.
    - exceptfds - in most cases set it to NULL.
    - if timeout is specified as NULL, select() blocks indefinitely waiting for a file descriptor 
      to become ready.
    - can monitor only file descriptors numbers that are less than FD_SETSIZE (1024).
    - each of the fd_set arguments may be specified as NULL if no file descriptors are to be watched for the corresponding class of events.
    - upon return, each of the file descriptor sets is modified in place to indicate which file descriptors are currently "ready". 
    - if using select() within a loop, the sets must be reinitialized before each call.
    - return the number of file descriptors contained in the three returned descriptor sets.
    - on error, -1 is returned, and errno is set to indicate the error.
*/

int pselect(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
    const struct timespec* timeout, const sigset_t* sigmask);
/* Desc:
    - allows an application to safely wait until either a file descriptor becomes ready or until a signal is caught.
    - if sigmask != NULL, then pselect() first replaces the current signal mask by the one pointed to by sigmask, then does the "select" function, and then restores the original signal mask.
    - is equivalent to the following calls invoked atomically:
      sigset_t origmask;
      pthread_sigmask(SIG_SETMASK, &sigmask, &origmask);
      ready = select(nfds, &readfds, &writefds, &exceptfds, timeout);
      pthread_sigmask(SIG_SETMASK, &origmask, NULL);
    - return the number of file descriptors contained in the three returned descriptor sets.
    - on error, -1 is returned, and errno is set to indicate the error.
    - errno == EINTR if a signal was caught.
*/

struct timeval {
   time_t      tv_sec;         /* seconds */
   suseconds_t tv_usec;        /* microseconds */
};
```

## `pool()` & `ppoll()`:
```c
#include <poll.h>

struct pollfd {
   int   fd;         /* file descriptor */
   short events;     /* requested events */
   short revents;    /* returned events */
};
/* Desc:
    - if fd field is negative, then the corresponding events field is ignored and the revents field returns zero.
    - events is a bit mask specyfing the events the application is interested in.
    - revents is an output parameter, filled by the kernel with the events that actually ocurred.

Common events:
- POOLIN - There is data to read.
- POOLOUT - Writing is now possible.
- POOLPRI - There is some exceptional condition (like OOD data on a TCP socket).
- POLLERR - Error condition. Set for a file descriptor referring to the write end of a pipe when the read end has been closed.
*/

int poll(struct pollfd *fds, nfds_t nfds, int timeout);
/* Desc:
    - waits for one of a set of file descriptors to become ready to perform I/O.
    - fds is an array of structures of type poolfd.
    - the caller should specify the number of items in the fds array in nfds.
    - returns a nonnegative value which is the number of elements in the pollfds whose revents fields have been set to a nonzero value (indicating an event or an error).
    - on error, -1 is returned, and errno is set to indicate the error.
*/

#define _GNU_SOURCE
int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec* tmo_p, const sigset_t* sigmask);
/* Desc:
    - allows an application to safely wait until either a file descriptor becomes ready or until a signal is caught.
    - analogous to the relation between select() and pselect().
    -  if tmo_p is specified as NULL, then ppoll() can block indefinitely.
    - errno == EINTR if a signal occurred before any requested event.
*/
```

## `epoll()`:

```c
/* Basic Theory:
The central concept of the epoll API is the epoll instance, an in-kernel data structure which,
from a user-space perspective, can be considered as a container for two lists:
- The interest list (sometimes also called the epoll set): 
  the set of file descriptors that the process has registered an interest in monitoring.
- The ready list: 
  the set of file descriptors that are "ready" for I/O.
  The ready list is a subset of (or, more precisely, a set of references to) the file descriptors in the interest list. The ready list is dynamically populated by the kernel as a result of I/O activity on those file descriptors.

The following system calls are provided to create and manage an epoll instance:
- epoll_create() - creates a new epoll instance and returns a file descriptor to that instance.
- epoll_ctl() - used to register or unregister a particular file descriptor.
- epoll_wait() - waits for I/O events, blocking the calling thread if no events are currently available.

Modes od epoll():
1. In level-triggered mode, an event will be reported until it is handled. 
This means that if a file descriptor is ready for reading or writing, epoll will notify 
about this event, but it will continue reporting this event in subsequent epoll_wait 
calls until it is handled by the application. For example, if data waiting to be read from
the file descriptor is not read, the next epoll_wait call will report the event again about
readiness to read.
2. In edge-triggered mode, an event will be reported only once when the state of the file
descriptor changes from not ready to ready. This means that epoll will notify of the event
only when it first notices that the state of the file descriptor has changed from unavailable
to available (e.g., the descriptor goes from not ready to ready for reading/writing). 
It will then not be reported again until the application handles that event and reacts 
to the state change. To instruct epoll to work in edge-triggered mode, you need to use the
EPOLLET flag when adding a file descriptor to the epoll instance. An application that 
employs the EPOLLET flag should use nonblocking file descriptors to avoid having a blocking
read or write starve a task that is handling multiple file descriptors. 
The suggested way to use epoll as an edge-triggered (EPOLLET) interface is as follows:
(1) with nonblocking file descriptors
(2) by waiting for an event only after read(2) or write(2) return EAGAIN.
*/

#include <sys/epoll.h>
int epoll_create(int size);
/* Desc:
    - size argument is ignored but should be > 0.
    - returns a file descriptor referring to the new epoll instance.
    - when no longer required, the file descriptor returned by epoll_create() should be closed by using close().
    - on error, -1 is returned, and errno is set to indicate the error.
*/

struct epoll_event {
   uint32_t      events;  /* Epoll events */
   epoll_data_t  data;    /* User data variable */
};
union epoll_data {
   void     *ptr;
   int       fd;
   uint32_t  u32;
   uint64_t  u64;
};
/* Desc:
    - specifies the data that the kernel should save and return when corresponding file descriptor becomes ready.
    - the events member of epoll_event is a bit mask of values:
        - EPOLLIN - associated fd is ready for read() operation.
        - EPOLLOUT - associated fd is ready for write() operation.
        - EPOLLRDHUP - stream socket peer closed connection, or shut down writing half of the connection.
        - EPOLLET - requests edge-triggered notification for associated fd.
        - EPOLLPRI - some exceptional condition on fd ocurred (similar to POLLPRI).
*/

int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event);
/* Desc:
    - controls interface for an epoll file descriptor.
    - used to add, modify, or remove entries in the 9nterest list of the epoll instance.
    - valid op calues are:
        - EPOLL_CTL_ADD - add an entry to the interest list of the epoll file descriptor.
        - EPOLL_CTL_MOD - change the settings associated with fd in the interest list to the new settings specified in event.
        - EPOLL_CTL_DEL - deregister the target file descriptor fd from the interest list. The event argument is ignored and can be set to NULL.
        - on success, returns 0.  
        - on error returns -1 and errno is set to indicate the error.
*/

int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
/* Desc:
    - the buffer pointed to by events is used to return information from the ready
      list about file descriptors in the interest list that have some events available.
    - returns the number of file descriptors ready for the requested I/O, 
      or zero if no file descriptor became ready during the requested timeout milliseconds.
    - on failure, returns -1 and errno is set to indicate the error.
    - specifying a timeout of -1 causes epoll_wait() to block indefinitely, while
      specifying a timeout equal to 0 cause epoll_wait() to return immediately, 
      even if no events are available.
*/

int epoll_pwait(int epfd, struct epoll_event *events, int maxevents, 
    int timeout, const sigset_t* sigmask);
/* Desc:
    - the relationship between epoll_wait() and epoll_pwait() is
      analogous to the relationship between select() and pselect():
*/
```

```c
int sop_setnonblock(int fd) {
  int oldflags = fcntl(fd, F_GETFL, 0);
  if (oldflags == -1) return -1;
  oldflags |= O_NONBLOCK;
  return fcntl(fd, F_SETFL, oldflags);
}
```

## `select()` example:
```c
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

struct sockaddr_in make_address(const char *address, const char *port) {
  int ret;
  struct sockaddr_in addr;
  struct addrinfo *result;
  struct addrinfo hints = {};
  hints.ai_family = AF_INET;
  if ((ret = getaddrinfo(address, port, &hints, &result))) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    exit(EXIT_FAILURE);
  }
  addr = *(struct sockaddr_in *)(result->ai_addr);
  freeaddrinfo(result);
  return addr;
}

#define MAX_CLIENTS 3

int set_nonblock(int desc) {
  int oldflags = fcntl(desc, F_GETFL, 0);
  if (oldflags == -1)
    return -1;
  oldflags |= O_NONBLOCK;
  return fcntl(desc, F_SETFL, oldflags);
}

void do_server(int server_socket) {

  int clients[MAX_CLIENTS];
  for (int i = 0; i < MAX_CLIENTS; ++i)
    clients[i] = -1;

  while (1) {
    fd_set rdset;
    FD_ZERO(&rdset);
    FD_SET(server_socket, &rdset);

    int maxfd = server_socket;
    for (int i = 0; i < MAX_CLIENTS; ++i) {
      if (clients[i] != -1) {
        FD_SET(clients[i], &rdset);
        if (clients[i] > maxfd)
          maxfd = clients[i];
      }
    }

    printf("Calling select()!\n");
    if (select(maxfd + 1, &rdset, NULL, NULL, NULL) < 0) ERR("select()");
    if (FD_ISSET(server_socket, &rdset)) {
      struct sockaddr client_addr;
      socklen_t client_addrlen = sizeof(client_addr);
      int client = accept(server_socket, &client_addr, &client_addrlen);
      if (client >= 0) {
        printf("Accepted new client!\n");
        set_nonblock(client);
        for (int i = 0; i < MAX_CLIENTS; ++i) {
          if (clients[i] == -1) {
            clients[i] = client;
            break;
          } else if (i == MAX_CLIENTS - 1) {
            char neat_reply[] = "No space for you :(\n";
            send(client, neat_reply, sizeof(neat_reply), 0);
            close(client);
          }
        }
      } else if (errno != EAGAIN) {
        ERR("accept()");
      }
    }

    for (int i = 0; i < MAX_CLIENTS; ++i) {
      if (FD_ISSET(clients[i], &rdset)) {
        char buff[16];
        ssize_t ret = recv(clients[i], buff, sizeof(buff), 0);
        printf("recv returned %ld bytes\n", ret);
        if (ret > 0) {
            printf("Client sent: '%.*s'\n", (int)ret, buff);
        } else if (ret == 0) {
            close(clients[i]);
            clients[i] = -1;
        } else if (errno != EAGAIN) {
            ERR("recv()");
        }
      }
    }
  }
}

int main(int argc, const char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "USAGE: %s host port\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  struct sockaddr_in addr = make_address(argv[1], argv[2]);
  int server_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (server_socket < 0) ERR("socket()");
  if (bind(server_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    ERR("bind()");
  if (listen(server_socket, 3) < 0) ERR("listen()");

  do_server(server_socket);
  close(server_socket);
  return 0;
}
```

## `pselect()` example:
```c
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

struct sockaddr_in make_address(const char *address, const char *port) {
  int ret;
  struct sockaddr_in addr;
  struct addrinfo *result;
  struct addrinfo hints = {};
  hints.ai_family = AF_INET;
  if ((ret = getaddrinfo(address, port, &hints, &result))) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    exit(EXIT_FAILURE);
  }
  addr = *(struct sockaddr_in *)(result->ai_addr);
  freeaddrinfo(result);
  return addr;
}

#define MAX_CLIENTS 3

int set_nonblock(int desc) {
  int oldflags = fcntl(desc, F_GETFL, 0);
  if (oldflags == -1)
    return -1;
  oldflags |= O_NONBLOCK;
  return fcntl(desc, F_SETFL, oldflags);
}

volatile sig_atomic_t interrupted = 0;

void signal_handler(int sig) { interrupted = 1; }

void do_server(int server_socket) {

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = signal_handler;
  if (sigaction(SIGINT, &sa, NULL) < 0) {
    perror("sigaction(): ");
    exit(EXIT_FAILURE);
  }

  sigset_t sigmask, old_sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGINT);
  sigprocmask(SIG_BLOCK, &sigmask, &old_sigmask);

  int clients[MAX_CLIENTS];
  for (int i = 0; i < MAX_CLIENTS; ++i)
    clients[i] = -1;

  while (1) {
    fd_set rdset;
    FD_ZERO(&rdset);
    FD_SET(server_socket, &rdset);

    int maxfd = server_socket;
    for (int i = 0; i < MAX_CLIENTS; ++i) {
      if (clients[i] != -1) {
        FD_SET(clients[i], &rdset);
        if (clients[i] > maxfd)
          maxfd = clients[i];
      }
    }

    printf("Calling pselect()!\n");
    int ret = pselect(maxfd + 1, &rdset, NULL, NULL, NULL, &old_sigmask);
    if (ret < 0) {
      if (errno == EINTR) {
        if (interrupted)
          break;
      }
      perror("select()");
      exit(EXIT_FAILURE);
    }

    if (FD_ISSET(server_socket, &rdset)) {
      struct sockaddr client_addr;
      socklen_t client_addrlen = sizeof(client_addr);
      int client = accept(server_socket, &client_addr, &client_addrlen);

      if (client >= 0) {
        printf("Accepted new client!\n");
        set_nonblock(client);
        for (int i = 0; i < MAX_CLIENTS; ++i) {
          if (clients[i] == -1) {
            clients[i] = client;
            break;
          } else if (i == MAX_CLIENTS - 1) {
            char neat_reply[] = "No space for you :(\n";
            send(client, neat_reply, sizeof(neat_reply), 0);
            close(client);
          }
        }
      } else if (errno != EAGAIN) {
        perror("accept()");
        exit(EXIT_FAILURE);
      }
    }

    for (int i = 0; i < MAX_CLIENTS; ++i) {
      if (FD_ISSET(clients[i], &rdset)) {
        char buff[16];
        ssize_t ret = recv(clients[i], buff, sizeof(buff), 0);
        printf("recv returned %ld bytes\n", ret);
        if (ret > 0) {
          printf("Client sent: '%.*s'\n", (int)ret, buff);
        } else if (ret == 0) {
          close(clients[i]);
          clients[i] = -1;
        } else if (errno != EAGAIN) {
          perror("recv()");
          exit(EXIT_FAILURE);
        }
      }
    }
  }

  for (int i = 0; i < MAX_CLIENTS; ++i) {
    if (clients[i] >= 0) {
      close(clients[i]);
    }
  }
}

int main(int argc, const char *argv[]) {

  if (argc != 3) {
    fprintf(stderr, "USAGE: %s host port\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in addr = make_address(argv[1], argv[2]);
  int server_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (server_socket < 0) {
    perror("socket()");
    exit(EXIT_FAILURE);
  }

  if (bind(server_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind()");
    exit(EXIT_FAILURE);
  }

  if (listen(server_socket, 3) < 0) {
    perror("listen()");
    exit(EXIT_FAILURE);
  }

  do_server(server_socket);

  close(server_socket);
  return 0;
}
```

## `poll()` example:
```c
#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

struct sockaddr_in make_address(const char *address, const char *port) {
  int ret;
  struct sockaddr_in addr;
  struct addrinfo *result;
  struct addrinfo hints = {};
  hints.ai_family = AF_INET;
  if ((ret = getaddrinfo(address, port, &hints, &result))) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    exit(EXIT_FAILURE);
  }
  addr = *(struct sockaddr_in *)(result->ai_addr);
  freeaddrinfo(result);
  return addr;
}

#define MAX_CLIENTS 3

int set_nonblock(int desc) {
  int oldflags = fcntl(desc, F_GETFL, 0);
  if (oldflags == -1)
    return -1;
  oldflags |= O_NONBLOCK;
  return fcntl(desc, F_SETFL, oldflags);
}

volatile sig_atomic_t interrupted = 0;

void signal_handler(int sig) { interrupted = 1; }

void do_server(int server_socket) {

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = signal_handler;
  if (sigaction(SIGINT, &sa, NULL) < 0) {
    perror("sigaction(): ");
    exit(EXIT_FAILURE);
  }

  sigset_t sigmask, old_sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGINT);
  sigprocmask(SIG_BLOCK, &sigmask, &old_sigmask);

  struct pollfd clients[MAX_CLIENTS];
  for (int i = 0; i < MAX_CLIENTS; ++i)
    clients[i].fd = -1;
  clients[0].fd = server_socket;
  clients[0].events = POLLIN;

  while (1) {
    printf("Calling ppoll()!\n");
    int ret = ppoll(clients, MAX_CLIENTS, NULL, &old_sigmask);
    if (ret < 0) {
      if (errno == EINTR) {
        if (interrupted)
          break;
      }
      perror("select()");
      exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_CLIENTS; ++i) {

      if (clients[i].fd == server_socket) {

        if (clients[i].revents & POLLIN) {

          struct sockaddr client_addr;
          socklen_t client_addrlen = sizeof(client_addr);
          int client = accept(server_socket, &client_addr, &client_addrlen);

          if (client >= 0) {
            printf("Accepted new client!\n");
            set_nonblock(client);
            for (int i = 0; i < MAX_CLIENTS; ++i) {
              if (clients[i].fd == -1) {
                clients[i].fd = client;
                clients[i].events = POLLIN;
                break;
              } else if (i == MAX_CLIENTS - 1) {
                char neat_reply[] = "No space for you :(\n";
                send(client, neat_reply, sizeof(neat_reply), 0);
                close(client);
              }
            }
          } else if (errno != EAGAIN) {
            perror("accept()");
            exit(EXIT_FAILURE);
          }
        }
      } else {

        if (clients[i].revents & POLLIN) {
          char buff[16];
          ssize_t ret = recv(clients[i].fd, buff, sizeof(buff), 0);
          printf("recv returned %ld bytes\n", ret);
          if (ret > 0) {
            printf("Client sent: '%.*s'\n", (int)ret, buff);
          } else if (ret == 0) {
            close(clients[i].fd);
            clients[i].fd = -1;
          } else if (errno != EAGAIN) {
            perror("recv()");
            exit(EXIT_FAILURE);
          }
        }
      }
    }
  }

  for (int i = 0; i < MAX_CLIENTS; ++i) {
    if (clients[i].fd >= 0) {
      close(clients[i].fd);
    }
  }
}

int main(int argc, const char *argv[]) {

  if (argc != 3) {
    fprintf(stderr, "USAGE: %s host port\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in addr = make_address(argv[1], argv[2]);
  int server_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (server_socket < 0) {
    perror("socket()");
    exit(EXIT_FAILURE);
  }

  if (bind(server_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind()");
    exit(EXIT_FAILURE);
  }

  if (listen(server_socket, 3) < 0) {
    perror("listen()");
    exit(EXIT_FAILURE);
  }

  do_server(server_socket);

  close(server_socket);
  return 0;
}
```

## `epoll()` example:
```c
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

struct sockaddr_in make_address(const char *address, const char *port) {
  int ret;
  struct sockaddr_in addr;
  struct addrinfo *result;
  struct addrinfo hints = {};
  hints.ai_family = AF_INET;
  if ((ret = getaddrinfo(address, port, &hints, &result))) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    exit(EXIT_FAILURE);
  }
  addr = *(struct sockaddr_in *)(result->ai_addr);
  freeaddrinfo(result);
  return addr;
}

#define MAX_CLIENTS 3

int set_nonblock(int desc) {
  int oldflags = fcntl(desc, F_GETFL, 0);
  if (oldflags == -1)
    return -1;
  oldflags |= O_NONBLOCK;
  return fcntl(desc, F_SETFL, oldflags);
}

void do_server(int server_socket) {

  int epoll_fd = epoll_create(100);

  struct epoll_event ev = {};
  ev.data.fd = server_socket;
  ev.events = EPOLLIN;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &ev) < 0) {
    perror("epoll_ctl()");
    exit(EXIT_FAILURE);
  }

  while (1) {

    struct epoll_event events[10];

    printf("Calling epoll_wait()!\n");
    int ret = epoll_wait(epoll_fd, events, 10, -1);
    if (ret < 0) {
      perror("epoll_wait()");
      exit(EXIT_FAILURE);
    }

    for (int i = 0; i < ret; ++i) {
      int fd = events[i].data.fd;
      if (events[i].events & EPOLLIN) {
        if (fd == server_socket) {
          struct sockaddr client_addr;
          socklen_t client_addrlen = sizeof(client_addr);
          int client = accept(server_socket, &client_addr, &client_addrlen);
          if (client >= 0) {
            printf("Accepted new client!\n");
            set_nonblock(client);
            ev.data.fd = client;
            ev.events = EPOLLIN;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client, &ev) < 0) {
              perror("epoll_ctl()");
              exit(EXIT_FAILURE);
            }
          } else if (errno != EAGAIN) {
            perror("accept()");
            exit(EXIT_FAILURE);
          }
        } else {
          char buff[16];
          ssize_t ret = recv(fd, buff, sizeof(buff), 0);
          printf("recv returned %ld bytes\n", ret);
          if (ret > 0) {
            printf("Client sent: '%.*s'\n", (int)ret, buff);
          } else if (ret == 0) {
            if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) < 0) {
              perror("epoll_ctl()");
              exit(EXIT_FAILURE);
            }
            close(fd);
          } else if (errno != EAGAIN) {
            perror("recv()");
            exit(EXIT_FAILURE);
          }
        }
      }
    }
  }
}

int main(int argc, const char *argv[]) {

  if (argc != 3) {
    fprintf(stderr, "USAGE: %s host port\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in addr = make_address(argv[1], argv[2]);
  int server_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (server_socket < 0) {
    perror("socket()");
    exit(EXIT_FAILURE);
  }

  if (bind(server_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind()");
    exit(EXIT_FAILURE);
  }

  if (listen(server_socket, 3) < 0) {
    perror("listen()");
    exit(EXIT_FAILURE);
  }
  do_server(server_socket);
  close(server_socket);
  return 0;
}
```