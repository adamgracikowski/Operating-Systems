#include "boardgame-utils.h"

void usage(char *name);
void parse_argv(int argc, char **argv, int *n);

int main(int argc, char **argv)
{
    int n;
    parse_argv(argc, argv, &n);

    sigset_t mask, old_mask;
    int signo[] = {SIGINT};
    sop_block_signals(&mask, &old_mask, signo, 1);

    pid_t pid = getpid();
    srand((unsigned)time(NULL) * getpid());
    printf("Server: My PID is %d\n", pid);

    char shm_name[SHM_NAME_MAX];
    create_shm_name(shm_name, SHM_NAME_MAX, pid);

    int shm_fd;
    if ((shm_fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, PERM)) < 0)
        ERR("shm_open");
    if (ftruncate(shm_fd, SHM_SIZE(n)) < 0)
        ERR("ftruncate");

    shm_data_t *shm_ptr;
    if ((shm_ptr = mmap(NULL, SHM_SIZE(n), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
        ERR("mmap");

    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            shm_ptr->board[i * n + j] = rand() % 9 + 1;

    pthread_mutexattr_t mutex_attr;
    sop_mutexattr_shared(&mutex_attr);
    if (pthread_mutex_init(&shm_ptr->board_mutex, &mutex_attr))
        ERR("pthread_mutex_init");
    if (pthread_mutex_init(&shm_ptr->counter_mutex, &mutex_attr))
        ERR("pthread_mutex_init");
    shm_ptr->counter = 0;

    int exit_flag = 0;
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

    sigthread_args_t sigthread_args = {.pmtx = &mtx, .pexit = &exit_flag, .mask = mask};
    pthread_t sigthread_tid;
    if (pthread_create(&sigthread_tid, NULL, sigthread_routine, &sigthread_args))
        ERR("pthread_create");

    sop_shm_subscribe(shm_ptr);

    while (1)
    {
        pthread_mutex_lock(&mtx);
        if (exit_flag)
        {
            pthread_mutex_unlock(&mtx);
            break;
        }
        pthread_mutex_unlock(&mtx);

        sop_mutex_sharedlock(&shm_ptr->board_mutex);
        print_board(shm_ptr->board, n);
        sop_mutex_sharedunlock(&shm_ptr->board_mutex);

        timespec_t ts = {.tv_sec = 3, .tv_nsec = 0};
        nanosleep(&ts, &ts);
    }

    if (pthread_join(sigthread_tid, NULL))
        ERR("pthread_join");
    printf("Server: Joined thread responsible for handling signals.\n");
    if (pthread_mutexattr_destroy(&mutex_attr))
        ERR("pthread_mutexattr_destroy");

    sop_shm_unsubscribe(shm_ptr);
    if (munmap(shm_ptr, SHM_SIZE(n)) < 0)
        ERR("munmap");
    if (shm_unlink(shm_name) < 0)
        ERR("shm_unlink");
    printf("Server: Unmapped and unlinked shared memory object.\n");

    return EXIT_SUCCESS;
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "%d <= n <= %d - board size\n", (int)MIN_N, (int)MAX_N);
    exit(EXIT_FAILURE);
}

void parse_argv(int argc, char **argv, int *n)
{
    if (argc != 2)
        usage(argv[0]);
    *n = atoi(argv[1]);
    if (*n < MIN_N || *n > MAX_N)
        usage(argv[0]);
}