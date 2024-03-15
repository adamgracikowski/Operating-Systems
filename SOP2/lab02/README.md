# Queues:
## Functions:
```c
/*  Basic Theory:
-   After a fork(), a child process inherits copies of its parent's message queue descriptors,
    and these descriptors refer to the same open message queue descriptions as the corresponding
    message queue descriptors in the parent.
-   Each message is assigned a priority, and messages are always delivered to the receiving
    process in order of highest priority first.
-   Message passing (of length 0 to mq_msgsize) is always reliable.
-   The queue persists system-wide, meaning it exists until system restart or explicit deletion.
-   The queue has finite capacity (mq_maxmsg messages).
-   Messages have a maximum length (mq_msgsize) specified at queue creation time.
-   Read operations must always be prepared to receive a message of maximum length.
-   When mq_maxmsg is exceeded, the process writing to the queue will be blocked
    (in blocking mode by default) until there is sufficient free space in the queue
    or until interrupted by a signal.
-   Messages can be assigned a priority (unsigned integer, smaller than the constant MQ_PRIO_MAX>=32).
-   Messages with the highest priority are placed at the front of the queue (in FIFO order).
-   Reading from an empty queue blocks the receiver (thread) if access is in blocking mode.
-   If multiple threads are blocked on mq_send()/mq_timedsend() due to a full queue,
    when space becomes available, the thread with the highest priority, waiting the longest, is unblocked.
*/

#include <mqueue.h>
mqd_t mq_open(const char *name, int oflag, ...);
/* Opis:
    - estabilishes a connection between a process and a message queue with a message queue descriptor.
    - name points to a string naming a message queue.
    - the name of the message queue should start with a slash.
    - oflag argument requests the desired receive and/or send access to the message queue.
    - returns a message queue descriptor, on error returns (mqd_t)-1 and sets errno to indicate the error.
    
Access Modes:
- O_RDONLY -   Open the message queue for receiving messages. The process can use the
               returned message queue descriptor with mq_receive(), but not mq_send().
               A message queue may be open multiple times in the same or different processes for receiving messages.
- O_WRONLY -   Open the queue for sending messages. The process can use the returned message queue descriptor
               with mq_send() but not mq_receive(). A message queue may be open multiple times in the same or
               different processes for sending messages.
- O_RDWR -     Open the queue for both receiving and sending messages. The process can use any of the functions
               allowed for O_RDONLY and O_WRONLY. A message queue may be open multiple times in the same or different
               processes for sending messages.
- O_CREAT -    Creates a message queue.
- O_EXCL -     If O_EXCL and O_CREAT are set, mq_open() shall fail if the message queue name exists.
- O_NONBLOCK - Determines whether an mq_send() or mq_receive() waits for resources or messages that are not
               currently available, or fails with errno set to [EAGAIN].
*/

int mq_close(mqd_t mqdes);
/* Opis:
    - closes a message queue.
    - removes the association between the message queue descriptor and its message queue.
    - returns 0, otherwise returns -1 and sets errno to indicate the error.
*/

int mq_unlink(const char *name);
/* Opis:
    - removes a message queue.
    - the actual destruction of a message queue is postponed until all references
      to the message queue have been closed.
    - returns 0, otherwise returns -1 and sets errno to indicate the error.
*/

// Message Queue Attributes:

struct mq_attr {
    long mq_flags;      // message queue description flags: 0 or O_NONBLOCK
    long mq_maxmsg;     // maximum number of messages in queue, fixed after creation
    long mq_msgsize;    // maximum message size (in bytes), fixed after creation
    long mq_curmsgs;    // number of messages currently in queue
}

int mq_getattr(mqd_t mqdes, struct mq_attr *mqstat);
/* Opis:
    - obtain status information and attributes of the message queue.
    - returns 0, otherwise returns -1 and sets errno to indicate the error.
*/

int mq_setattr(mqd_t mqdes, const struct mq_attr* mqstat, struct mq_attr* omqstat);
/* Opis:
    - sets attributes associated with the open message queue.
    - this function can change only mq_flags, which can be set to for example: O_NONBLOCK.
    - values of the mq_maxmsg, mq_msgsize, and mq_curmsgs of the mq_attr structure are ignored by mq_setattr().
    - if omqstat != NULL, then previous attributes of the message queue are stored there.
    - returns 0, otherwise returns -1 and sets errno to indicate the error.
*/

// Exchanging Messages:

int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned msg_prio);
/* Opis:
    - adds the message pointed to by the argument msg_ptr to the message queue specified by mqdes.
    - value of msg_len shall be less than or equal to the mq_msgsize attribute of the message queue,
      or mq_send() shall fail.
    - message with a larger numeric value of msg_prio shall be inserted before messages with lower v
      alues of msg_prio.
    - returns 0, otherwise returns -1 and sets errno to indicate the error.

If the specified message queue is full and O_NONBLOCK is not set in the message queue description
associated with mqdes, mq_send() shall block until space becomes available to enqueue the message,
or until mq_send() is interrupted by a signal. If more than one thread is waiting to send when space
becomes available in the message queue and the Priority Scheduling option is supported, then the thread
of the highest priority that has been waiting the longest shall be unblocked to send its message.
If the specified message queue is full and O_NONBLOCK is set in the message queue description
associated with mqdes, the message shall not be queued and mq_send() shall return an error.
*/

#include <mqueue.h>
#include <time.h>
int mq_timedsend(mqd_t mqdes, const char *msg_ptr, size_t msg_len, 
    unsigned msg_prio, const struct timespec *abstime);     
/* Opis:
    - _XOPEN_SOURCE 600
    - behaves similar to the mq_send() function.
    - the wait for sufficient room in the queue shall be terminated when the specified timeout expires.
    - the timeout shall be based on the CLOCK_REALTIME clock.
    - returns 0, otherwise returns -1 and sets errno to indicate the error.
    
Common errors:
- EAGAIN -    The O_NONBLOCK flag is set in the message queue description associated with mqdes,
              and the specifie message queue is full.
- EINTR -     A signal interrupted the call to mq_send() or mq_timedsend().
- ETIMEDOUT - The O_NONBLOCK flag was not set when the message queue was opened, but the timeout
              expired before the message could be added to the queue.
- EMSGSIZE -  If msg_len is greater than mq_msgsize.
*/

ssize_t mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned *msg_prio);
/* Opis:
    - receives the oldest of the highest priority message from the message queue.
    if msg_prio != NULL, the priority of the message is stored there.
    - if msg_len < mq_msgsize than mq_receive() fails with EMSGSIZE.

If the specified message queue is empty and O_NONBLOCK is not set in the message queue description
associated with mqdes, mq_receive() shall block until a message is enqueued on the message queue
or until mq_receive() is interrupted by a signal.
If more than one thread is waiting to receive a message when a message arrives at an empty queue
and the Priority Scheduling option is supported, then the thread of highest priority that has been
waiting the longest shall be selected to receive the message.
If the specified message queue is empty and O_NONBLOCK is set in the message queue description
associated with mqdes, no message shall be removed from the queue, and mq_receive() shall return an error.
*/

#include <mqueue.h>
#include <time.h>
ssize_t mq_timedreceive(mqd_t mqdes, char* msg_ptr, size_t msg_len, unsigned* msg_prio, 
    const struct timespec* abstime);
/* Opis:
    - _XOPEN_SOURCE 600
    - behaves similar to mq_erceive() function.
    - the wait for such a message shall be terminated when the specified timeout expires.
    - the timeout shall be based on the CLOCK_REALTIME clock.
    - returns 0, otherwise returns -1 and sets errno to indicate the error.
*/

int mq_notify(mqd_t mqdes, const struct sigevent *notification);
/* Opis:
    - if the argument notification is not NULL, this function shall register the calling process
      to be notified of message arrival at an empty message queue associated with the specified
      message queue descriptor, mqdes.
    - the notification specified by the notification argument shall be sent to the process when
      the message queue transitions from empty to non-empty.
    - at any time, only one process may be registered for notification by a message queue.
    - if the calling process or any other process has already registered for notification of
      message arrival at the specified message queue, subsequent attempts to register for that
      message queue shall fail with EBUSY.
    - if notification is NULL and the process is currently registered for notification by the
      specified message queue, the existing registration shall be removed.
    - the registered process is notified only if some other process is not currently blocked
      in a call to mq_receive() for the queue. If some other process is blocked in mq_receive(),
      that process will read the message, and the registered process will remain registered.
    - returns 0, otherwise returns -1 and sets errno to indicate the error.
*/

#include <signal.h>
union sigval {           /* Data passed with notification */
   int     sival_int;    /* Integer value */
   void   *sival_ptr;    /* Pointer value */
};

struct sigevent {
   int    sigev_notify;  /* Notification method */
   int    sigev_signo;   /* Notification signal */
   union sigval sigev_value; /* Data passed with notification */
   void (*sigev_notify_function)(union sigval); 
   /* Function used for thread notification (SIGEV_THREAD) */
   void  *sigev_notify_attributes; 
   /* Attributes for notification thread (SIGEV_THREAD) */
   pid_t  sigev_notify_thread_id;
  /* ID of thread to signal (SIGEV_THREAD_ID); Linux-specific */
};

/*
The sigev_notify is set to one of the following values:
- SIGEV_NONE -   when a message arrives on the previously empty queue, don't actually notify the process.
- SIGEV_SIGNAL - notify the process by generating the signal specified in the sigev_signo field.
- SIGEV_THREAD - notify the process by calling the function specified in sigev_notify_function
                 as if it were a start function in a new thread. The sigev_notify_attributes field
                 can be specified as NULL or as a pointer to a pthread_attr_t structure that defines
                 attributes for the thread. The union sigval value specified in sigev_value is passed
                 as the argument of this function.
*/
```
