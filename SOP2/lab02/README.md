# Queues:
## Funkcje:
```c
/*
After a fork(), a child inherits copies of its parent's message queue descriptors, and these descriptors refer to the same open message queue descriptions as the corresponding message queue descriptors in the parent.

Each message has an associated priority, and messages are always delivered to the receiving process highest priority first.

Przekazywanie komunikatów (o długości 0 do mq_msgsize) jest niezawodne.
Kolejka ma trwałość w ramach systemu, tzn. istnieje do restartu systemu lub do 
jawnego usunięcia.
Kolejka ma skończoną pojemność (mq_maxmsg komunikatów).
Komunikaty mają długość maksymalną (mq_msgsize) określoną w czasie tworzenia kolejki. Operacje odczytu muszą być zawsze przygotowane na odbiór 
komunikatu o maksymalnej długości.
Gdy mq_maxmsg zostanie ona przekroczona - proces piszący do kolejki będzie zablokowany (przy pracy w domyślnym trybie: blokującym) - aż będzie dostatecznie dużo miejsca wolnego w kolejce, bądź do przerwania sygnałem.
Komunikatom można nadać priorytet (liczba całkowita bez znaku, mniejsza od stałej MQ_PRIO_MAX>=32, do pobrania przez sysconf()). Komunikaty o najwyższym priorytecie są umieszczane na początku kolejki (w porządku FIFO).
Operacja odczytu z pustej kolejki blokuje odbiorcę (wątek), jeśli dostęp jest w trybie z blokowaniem.
Jeśli kilka wątków blokuje na mq_send()/mq_timedsend() z powodu pełnej kolejki, to przy zwolnieniu miejsca odblokowywany jest wątek o najwyższym priorytecie, czekający najdłużej.

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
- O_RDONLY - Open the message queue for receiving messages. The
process can use the returned message queue descriptor with mq_receive(), but not mq_send(). A message queue may be open multiple times in the same or different processes for receiving messages.
- O_WRONLY - Open the queue for sending messages. The process can use the returned message queue descriptor with mq_send() but not mq_receive(). A message queue may be open multiple times in the same or different processes for sending messages.
- O_RDWR - Open the queue for both receiving and sending messages. The process can use any of the functions allowed for O_RDONLY and O_WRONLY. A message queue may be open multiple times in the same or different processes for sending messages.
- O_CREAT - creates a message queue.
- O_EXCL - If O_EXCL and O_CREAT are set, mq_open() shall fail if the message queue name exists.
- O_NONBLOCK - Determines whether an mq_send() or mq_receive() waits for resources or messages that are not currently available, or fails with errno set to [EAGAIN].
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
    - the actual destruction of a message queue is postponed until all references to the message queue have been closed.
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
    - value of msg_len shall be less than or equal to the mq_msgsize attribute of the message queue, or mq_send() shall fail.
    - message with a larger numeric value of msg_prio shall be inserted before messages with lower values of msg_prio.
    - returns 0, otherwise returns -1 and sets errno to indicate the error.

If the specified message queue is full and O_NONBLOCK is not set in the message queue description associated with mqdes, mq_send() shall block until space becomes available to enqueue the message, or until mq_send() is interrupted by a signal. If more than one thread is waiting to send when space becomes available in the message queue and the Priority Scheduling option is supported, then the thread of the highest priority that has been waiting the longest shall be unblocked to send its message.
If the specified message queue is full and O_NONBLOCK is set in the message queue description associated with mqdes, the message shall not be queued and mq_send() shall return an error.
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
- EAGAIN - The O_NONBLOCK flag is set in the message queue description associated with mqdes, and the specifie message queue is full.
- EINTR - A signal interrupted the call to mq_send() or mq_timedsend().
- ETIMEDOUT - The O_NONBLOCK flag was not set when the message queue was opened, but the timeout expired before the message could be added to the queue.
- EMSGSIZE - If msg_len is greater than mq_msgsize.
*/

ssize_t mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned *msg_prio);
/* Opis:
    - receives the oldest of the highest priority message from the message queue.
    if msg_prio != NULL, the priority of the message is stored there.
    - if msg_len < mq_msgsize than mq_receive() fails with EMSGSIZE.

If the specified message queue is empty and O_NONBLOCK is not set in the message queue description associated with mqdes, mq_receive() shall block until a message is enqueued on the message queue or until mq_receive() is interrupted by a signal.
If more than one thread is waiting to receive a message when a message arrives at an empty queue and the Priority Scheduling option is supported, then the thread of highest priority that has been waiting the longest shall be selected to receive the message.
If the specified message queue is empty and O_NONBLOCK is set in the message queue description associated with mqdes, no message shall be removed from the queue, and mq_receive() shall return an error.
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
    - if the argument notification is not NULL, this function shall register the calling process to be notified of message arrival at an empty message queue associated with the specified message queue descriptor, mqdes.
    - the notification specified by the notification argument shall be sent to the process when the message queue transitions from empty to non-empty.
    - at any time, only one process may be registered for notification by a message queue.
    - if the calling process or any other process has already registered for notification of message arrival at the specified message queue, subsequent attempts to register for that message queue shall fail with EBUSY.
    - if notification is NULL and the process is currently registered for notification by the specified message queue, the existing registration shall be removed.
    - the registered process is notified only if some other process is not currently blocked in a call to mq_receive() for the queue. If some other process is blocked in mq_receive(), that process will read the message, and the registered process will remain registered.
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
- SIGEV_NONE - when a message arrives on the previously empty queue, don't actually notify the process.
- SIGEV_SIGNAL - notify the process by generating the signal specified in the sigev_signo field.
- SIGEV_THREAD - notify the process by calling the function specified in sigev_notify_function as if it were a start function in a new thread. The sigev_notify_attributes field can be specified as NULL or as a pointer to a pthread_attr_t structure that defines attributes for the thread. The union sigval value specified in sigev_value is passed as the argument of this function.
*/
```

## Przykłady:
```c
mq_unlink("/sop_mqueue");

mqd_t mq = mq_open("/sop_mqueue", O_CREAT | O_RDWR, 0600, NULL);
if (mq < 0) ERR("mq_open")

if (mq_send(mq, "text", 5, 3) < 0) ERR("mq_send");

char *buf = malloc(BUF_SIZE);
unsigned int prio;
ssize_t ret = mq_receive(mq, buf, BUF_SIZE, &prio);
if (ret < 0) ERR("mq_receive");

printf("Received message of size %ld (prio: %u): '%s'", ret, prio, buf);
free(buf);

if(mq_close(mq) < 0) ERR("mq_close");

struct mq_attr attr;
mq_getattr(mq, &attr);
printf("attr.mq_maxmsg = %ld\n", attr.mq_maxmsg);
printf("attr.mq_msgsize = %ld\n", attr.mq_msgsize);

char *buf = (char *) malloc(attr.mq_msgsize);
unsigned int prio;
ssize_t ret = mq_receive(mq, buf, attr.mq_msgsize, &prio);
if (ret < 0) ERR("mq_receive");
    
#define MSG_MAX 3
#define MSG_SIZE 64

struct mq_attr attr;
memset(&attr, 0, sizeof(attr));
attr.mq_msgsize = MSG_SIZE;
attr.mq_maxmsg = MSG_MAX;

mqd_t mq = mq_open("/sop_mqueue", O_CREAT | O_RDWR, 0600, &attr);
if (mq < 0) ERR("mq_open");

char buf[MSG_SIZE]; // after adjustments, the buffer can be small
unsigned int prio;

ssize_t ret = mq_receive(mq, buf, sizeof(buf), &prio);
if (ret < 0) ERR("mq_receive");

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <mqueue.h>
#include <sys/wait.h>

#define MSG_MAX 10
#define MSG_SIZE 64

int main() {
    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_msgsize = MSG_SIZE;
    attr.mq_maxmsg = MSG_MAX;

    mq_unlink("/sop_mqueue");

    switch (fork()) {
        case -1:
            ERR("fork");
        case 0: {
            mqd_t mq = mq_open("/sop_mqueue", O_CREAT | O_RDONLY, 0600, &attr);
            if (mq < 0) ERR("mq_open");
            char buf[MSG_SIZE];
            unsigned int prio;
            
            int i = 0;
            while (1) {
                if (mq_receive(mq, buf, sizeof(buf), &prio) < 0) ERR("mq_receive");
                printf("Received message %d\n", i);
                i++;
            }
            mq_close(mq);
            break;
        }
        default: {
            mqd_t mq = mq_open("/sop_mqueue", O_CREAT | O_WRONLY, 0600, &attr);
            if (mq < 0) ERR("mq_open");

            for (int i = 0; i < 5; ++i) {
                printf("Sending message %d\n", i);
                if (mq_send(mq, "text", 5, 7) < 0) eRR("mq_send");
                sleep(1);
            }
            mq_close(mq);
            printf("Parent done! Waiting for child...\n");
            while (wait(NULL) > 0);
            break;
        }
    }
    return 0;
}

case 0: {
    mqd_t mq = mq_open("/sop_mqueue", O_CREAT | O_RDONLY, 0600, &attr);
    if (mq < 0) ERR("mq_open");

    char buf[MSG_SIZE];
    unsigned int prio;

    int i = 0;
    while (1) {
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) < 0) ERR("clock_gettime");
        ts.tv_sec += 3; // 3 seconds from now!

        if (mq_timedreceive(mq, buf, sizeof(buf), &prio, &ts) < 0) {
            if (errno == ETIMEDOUT) {
                printf("Timeout!\n");
                mq_close(mq);
                return 0;
            }
            ERR("mq_timedreceive");
        }
        printf("Received message %d\n", i);
        i++;
    }
}

srand(getpid());
int prio = (rand() % MQ_PRIO_MAX);

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mqueue.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

#define MSG_MAX 10
#define MSG_SIZE 64

volatile sig_atomic_t should_exit = 0;

void handler(int sig, siginfo_t *s, void *context) {
    printf("Received signal %d\n", sig);
}

void sigint_handler() {
    should_exit = 1;
}

int main() {

    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_msgsize = MSG_SIZE;
    attr.mq_maxmsg = MSG_MAX;

    mq_unlink("/sop_mqueue");

    mqd_t mq = mq_open("/sop_mqueue", O_CREAT | O_EXCL | O_RDWR | O_NONBLOCK, 0600, &attr);
    if (mq < 0) {
        perror("mq_open()");
        return 1;
    }

    // Setup handling of notification signal
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = handler;
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        perror("sigaction()");
        return 1;
    }

    // Setup SIGINT notification
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction()");
        return 1;
    }

    // Setup (first) notification - this is not inherited!
    struct sigevent not;
    memset(&not, 0, sizeof(not));
    not.sigev_notify = SIGEV_SIGNAL;
    not.sigev_signo = SIGUSR1;
    if (mq_notify(mq, &not) < 0) {
        perror("mq_notify()");
        return 1;
    }

    switch (fork()) {
        case -1:
            perror("fork()");
            return 1;
        case 0: {
            // child

            srand(getpid());

            // Send 5 blocks of messages
            for (int i = 0; i < 5; ++i) {
                int n = rand() % 3 + 1;
                printf("Sending %d messages\n", n);
                for (int j = 0; j < n; ++j) {
                    if (mq_send(mq, "text", 5, 0) < 0) {
                        perror("mq_send()");
                        return 1;
                    }
                }
                sleep(1);
            }

            kill(getppid(), SIGINT);
            break;
        }
        default: {
            // parent

            sigset_t set, oldset;
            sigemptyset(&set);
            sigaddset(&set, SIGUSR1);
            sigaddset(&set, SIGINT);
            if (sigprocmask(SIG_BLOCK, &set, &oldset) < 0) {
                perror("sigprocmask()");
                return 1;
            }

            while (sigsuspend(&oldset) < 0) {
                if (should_exit)
                    break;

                // Restore notification
                // TODO: Comment that out
                if (mq_notify(mq, &not) < 0) {
                    perror("mq_notify()");
                    return 1;
                }

                char buf[MSG_SIZE];
                unsigned int prio;

                // Empty the queue
                while (1) {
                    ssize_t ret = mq_receive(mq, buf, sizeof(buf), &prio);
                    if (ret < 0) {
                        if (errno == EAGAIN) break;
                        perror("mq_receive()");
                        return 1;
                    } else {
                        printf("Received message (prio: %u): '%s'\n", prio, buf);
                    }
                }

            }

            while (wait(NULL) > 0);
            break;
        }
    }

    mq_close(mq);

    return 0;
}

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mqueue.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

#define MSG_MAX 10
#define MSG_SIZE 64

volatile sig_atomic_t should_exit = 0;

void sigint_handler() {
    should_exit = 1;
}

void thread_routine(union sigval sv) {
    printf("Notification thread starts!\n");

    mqd_t *mq_ptr = (mqd_t *) sv.sival_ptr;

    // Restore notification
    struct sigevent not;
    memset(&not, 0, sizeof(not));
    not.sigev_notify = SIGEV_THREAD;
    not.sigev_notify_function = thread_routine;
    not.sigev_notify_attributes = NULL; // Thread creation attributes
    not.sigev_value.sival_ptr = mq_ptr; // Thread routine argument
    if (mq_notify(*mq_ptr, &not) < 0) {
      perror("mq_notify()");
      exit(1);
    }

    char buf[MSG_SIZE];
    unsigned int prio;

    // Empty the queue
    while (1) {
        ssize_t ret = mq_receive(*mq_ptr, buf, sizeof(buf), &prio);
        if (ret < 0) {
            if (errno == EAGAIN) break;
            perror("mq_receive()");
            exit(1);
        } else {
            printf("Received message (prio: %u): '%s'\n", prio, buf);
        }
    }

}

int main() {

    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_msgsize = MSG_SIZE;
    attr.mq_maxmsg = MSG_MAX;

    mq_unlink("/sop_mqueue");

    mqd_t mq = mq_open("/sop_mqueue", O_CREAT | O_EXCL | O_RDWR | O_NONBLOCK, 0600, &attr);
    if (mq < 0) {
        perror("mq_open()");
        return 1;
    }

    // Setup SIGINT notification
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction()");
        return 1;
    }

    // Setup (first) notification - this is not inherited!
    struct sigevent not;
    memset(&not, 0, sizeof(not));
    not.sigev_notify = SIGEV_THREAD;
    not.sigev_notify_function = thread_routine;
    not.sigev_notify_attributes = NULL; // Thread creation attributes
    not.sigev_value.sival_ptr = &mq; // Thread routine argument
    if (mq_notify(mq, &not) < 0) {
        perror("mq_notify()");
        return 1;
    }

    switch (fork()) {
        case -1:
            perror("fork()");
            return 1;
        case 0: {
            // child

            srand(getpid());

            // Send 5 blocks of messages
            for (int i = 0; i < 5; ++i) {
                int n = rand() % 3 + 1;
                printf("Sending %d messages\n", n);
                for (int j = 0; j < n; ++j) {
                    if (mq_send(mq, "text", 5, 0) < 0) {
                        perror("mq_send()");
                        return 1;
                    }
                }
                sleep(1);
            }

            kill(getppid(), SIGINT);
            break;
        }
        default: {
            // parent

            sigset_t set, oldset;
            sigemptyset(&set);
            sigaddset(&set, SIGUSR1);
            sigaddset(&set, SIGINT);
            if (sigprocmask(SIG_BLOCK, &set, &oldset) < 0) {
                perror("sigprocmask()");
                return 1;
            }

            while (sigsuspend(&oldset) < 0) {
                if (should_exit)
                    break;
            }

            while (wait(NULL) > 0);
            break;
        }
    }

    mq_close(mq);

    return 0;
}

// Odbieranie informacji o pojawieniu się wiadomości poprzez sygnał:
#include <signal.h>
#include <mqueue.h>
#include <fcntl.h>

#define UNUSED(x) (void)(x)
#define NOTIFY_SIG SIGUSR1

void handler(int sig){
    UNUSED(sig); // just interupt sigsuspend()
}

int main(int argc, char** argv){
    struct sigevent sev;
    mqd_t mqd;
    struct mq_attr attr;
    void* buffer;
    ssize_t numRead;
    sigset blockMask, emptyMask;
    struct sigaction sa;
    
    mqd = mq_open(argv[1], O_RDONLY | O_NONBLOCK);
    if(mqd == (mqd_t)-1) ERR("mq_open");
    
    if(mq_getattr(mqd, &attr) < 0) ERR("mq_getattr");
    if((buffer = malloc(attr.mq_msgsize)) == NULL) ERR("malloc");
    
    sigemptyset(&blockMask);
    sigaddset(&blockMask, NOTIFY_SIG);
    if(sigprocmask(SIG_BLOCK, &block_mask, NULL) < 0) ERR("sigprocmask");
    sigemtyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = handler;
    if(sigaction(NOTIFY_SIG, &sa, NULL) < 0) ERR("sigaction");
    
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.segev_signo = NOTIFY_SIG;
    if(mq_notify(mqd, &sev) < 0) ERR("mq_notify");
    
    sigemptyset(&emtyMask);
    for(;;){
        sigsuspend(&emptyMask);
        if(mq_notify(mqd, &sev) < 0) ERR("mq_notify");
        while((numRead = mq_receive(mqd, buffer, attr.mq_msgsize, NULL) >= 0)
            printf("Read %ld bytes.\n", (long)numRead);
        if(errno != EAGAIN)
            ERR("mq_receive");
    }
    
}
```
